
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "csx_assert.h"
#include "error.h"
#include "os.h"
#include "smfile.h"
#include "stdtypes.h"

#define CHUNK_SIZE 4096

/*
 * CAUTION: This is not error handled
 */
int
main (int argc, char **argv)
{
  if (argc != 5 && argc != 6)
  {
    fprintf (
        stdout,
        "Usage: big_file_inner_insert <FILE SIZE> <OFFSET> <INSERT SIZE> "
        "<CHUNK SIZE> [--verbose]"
    );
    return EXIT_FAILURE;
  }

  const u64 fsize                  = (u64)atoi (argv[1]);
  const u64 ofst                   = (u64)atoi (argv[2]);
  const u64 insize                 = (u64)atoi (argv[3]);
  const u64 chunk_size             = (u64)atoi (argv[4]);
  double    file_buffered_result   = 0;
  double    file_unbuffered_result = 0;
  double    ns_result              = 0;

  if (ofst > fsize)
  {
    fprintf (stderr, "offset %llu exceeds file size %llu\n", ofst, fsize);
    return -1;
  }

  error      e          = error_create ();
  const char filename[] = "temp_data.bin";

  unsigned char *seed   = malloc (fsize);
  unsigned char *insert = malloc (insize);

  for (u64 i = 0; i < fsize; i++)
  {
    seed[i] = (unsigned char)(i % 256);
  }
  for (u64 i = 0; i < insize; i++)
  {
    insert[i] = (unsigned char)(i % 256);
  }

  // UNBUFFERED
  {
    // Create and open file
    i_file fp;
    i_touch (filename, &e);
    i_open_rw (&fp, filename, &e);
    i_pwrite_all (&fp, seed, fsize, 0, &e);

    // Let the unbuffered version cheat and allocate ahead of time
    // this is letting it skip the system call to get fsize
    // numstore doesn't need that
    u64            tail_size = fsize - ofst;
    unsigned char *tail      = malloc (tail_size);

    // Timed segment
    {
      i_timer timer;
      i_timer_create (&timer, &e);

      // Start
      u64 start = i_timer_now_ns (&timer);

      {
        i_pread_all_expect (&fp, tail, tail_size, ofst, &e);
        i_pwrite_all (&fp, tail, tail_size, ofst + insize, &e);
        i_pwrite_all (&fp, insert, insize, ofst, &e);
        i_fsync (&fp, &e);
      }

      // Done
      u64 end = i_timer_now_us (&timer);
      ASSERT (end > start);
      file_unbuffered_result = (double)(end - start) * 0.001;
    }

    i_close (&fp, &e);
    i_remove_quiet (filename, &e);
    free (tail);
  }

  // BUFFERED
  {
    // Create and open file
    i_file fp;
    i_touch (filename, &e);

    i_open_rw (&fp, filename, &e);
    i_pwrite_all (&fp, seed, fsize, 0, &e);

    // Prepare
    // Pre allocate buffer ahead of time
    unsigned char *buffer = malloc (chunk_size);

    // Timed segment
    {
      i_timer timer;
      i_timer_create (&timer, &e);

      // Start
      u64 start = i_timer_now_ns (&timer);
      {
        u64 remaining = fsize - ofst;
        u64 read_pos  = fsize;

        // Read in chunks
        while (remaining > 0)
        {
          const u64 chunk = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
          read_pos -= chunk;

          i_pread_all_expect (&fp, buffer, chunk, read_pos, &e);
          i_pwrite_all (&fp, buffer, chunk, read_pos + insize, &e);

          remaining -= chunk;
        }
      }
      i_pwrite_all (&fp, insert, insize, ofst, &e);
      i_fsync (&fp, &e);

      // Done
      u64 end = i_timer_now_us (&timer);
      ASSERT (end > start);
      file_buffered_result = (double)(end - start) * 0.001;
    }

    i_close (&fp, &e);
    i_remove_quiet (filename, &e);
    free (buffer);
  }

// OPTIMIZED
#if 0
  {
    i_file fp;
    i_touch (filename, &e);
    i_open_rw (&fp, filename, &e);
    i_pwrite_all (&fp, seed, fsize, 0, &e);

    i_timer timer;
    i_timer_create (&timer, &e);
    u64 start = i_timer_now_ns (&timer);

    {
      int fd        = fp.fd; /* or however your i_file exposes the raw fd */
      u64 tail_size = fsize - ofst;

      /*
       * Fast path: ask the kernel to insert a hole of exactly insize bytes
       * starting at ofst, shifting all extents after that point forward.
       * On ext4/XFS this is an extent-tree pointer update — zero bytes of
       * data are moved. Falls back to EOPNOTSUPP on tmpfs/FAT/etc.
       *
       * Requires: offset and length both aligned to filesystem block size.
       * We add a runtime check and fall through to the copy path if either
       * alignment requirement isn't met.
       */
      int used_fallocate = 0;

#  ifdef __linux__
      {
        /* Probe block size via statfs so we work on any fs. */
        struct statfs sfs;
        long          blksz = (fstatfs (fd, &sfs) == 0) ? sfs.f_bsize : 4096;

        if ((ofst % blksz == 0) && (insize % blksz == 0))
        {
          if (fallocate (fd, FALLOC_FL_INSERT_RANGE, (off_t)ofst, (off_t)insize)
              == 0)
          {
            /* Kernel has already shifted the tail. Just write insert. */
            used_fallocate = 1;
          }
        }
      }
#  endif

      if (!used_fallocate)
      {
        /*
         * Fallback: copy_file_range moves data kernel-side with no
         * user-space buffer.  We use two logical "slots" and issue the
         * copy back-to-front (same overlap-safe direction as the buffered
         * approach) but the data never crosses the user/kernel boundary.
         *
         * copy_file_range is available on Linux 4.5+ and glibc 2.27+.
         * For portability the double-buffer preadv/pwritev path below
         * serves as the second fallback.
         */
        int used_cfr = 0;

#  if defined(__linux__) && defined(__NR_copy_file_range)
        {
          /* Extend file to final size first so we can write in place. */
          if (ftruncate (fd, (off_t)(fsize + insize)) == 0)
          {
            u64   remaining = tail_size;
            off_t src_pos   = (off_t)fsize; /* read from the end */
            off_t dst_pos   = (off_t)(fsize + insize);
            used_cfr        = 1;

            while (remaining > 0)
            {
              const u64 chunk = remaining < chunk_size ? remaining : chunk_size;
              src_pos -= (off_t)chunk;
              dst_pos -= (off_t)chunk;

              /*
               * copy_file_range takes pointers-to-offset so it can
               * advance them; we pass explicit positions instead.
               */
              off_t   s = src_pos, d = dst_pos;
              ssize_t copied = copy_file_range (fd, &s, fd, &d, chunk, 0);
              if (copied < 0)
              {
                used_cfr = 0;
                break;
              }
              remaining -= (u64)copied;
            }
          }
        }
#  endif

        if (!used_cfr)
        {
          /*
           * Final fallback: double-buffered preadv/pwritev.
           *
           * Two equal-sized buffers, A and B, alternate roles each
           * iteration: while buffer A's freshly-read chunk is being
           * written, buffer B issues the next read.  On a single-queue
           * block device this doesn't help (writes block reads); on
           * NVMe or io_uring it keeps the queue full.
           *
           * The simplified synchronous version below is correct
           * everywhere.  Swap out the preadv/pwritev pairs for
           * io_uring_prep_readv / io_uring_prep_writev submissions
           * if you want true overlap on NVMe.
           */
          unsigned char *buf_a = malloc (chunk_size);
          unsigned char *buf_b = malloc (chunk_size);

          /* Grow file to final size. */
          ftruncate (fd, (off_t)(fsize + insize));

          u64   remaining = tail_size;
          off_t read_pos  = (off_t)fsize;
          off_t write_pos = (off_t)(fsize + insize);
          int   flip      = 0;

          while (remaining > 0)
          {
            const u64 chunk = remaining < chunk_size ? remaining : chunk_size;
            unsigned char *buf = flip ? buf_b : buf_a;

            read_pos -= (off_t)chunk;
            write_pos -= (off_t)chunk;

            struct iovec riov = {buf, chunk};
            struct iovec wiov = {buf, chunk};

            /* Read this chunk. */
            ssize_t nr = preadv (fd, &riov, 1, read_pos);
            ASSERT ((u64)nr == chunk);

            /* Write it shifted. */
            ssize_t nw = pwritev (fd, &wiov, 1, write_pos);
            ASSERT ((u64)nw == chunk);

            remaining -= chunk;
            flip ^= 1;
          }

          free (buf_a);
          free (buf_b);
        }
      }

      /* Write the actual inserted payload. */
      i_pwrite_all (&fp, insert, insize, ofst, &e);
      i_fsync (&fp, &e);
    }

    u64 end = i_timer_now_us (&timer);
    ASSERT (end > start);
    double opt_result = (double)(end - start) * 0.001;

    i_close (&fp, &e);
    i_remove_quiet (filename, &e);
  }
#endif

  // SMARTFILES
  {
    smfile_t *file = smfile_open (filename);

    smfile_insert (file, seed, 0, fsize);

    // Timed segment
    {
      i_timer timer;
      i_timer_create (&timer, &e);

      // Start
      u64 start = i_timer_now_ns (&timer);

      smfile_insert (file, insert, ofst, insize);

      // Done
      u64 end = i_timer_now_us (&timer);
      ASSERT (end > start);
      ns_result = (double)(end - start) * 0.001;
    }

    smfile_close (file);
    i_remove_quiet (filename, &e);
  }

  free (insert);
  free (seed);

  if (argc == 6 && strncmp (argv[5], "--verbose", 9) == 0)
  {
    printf (
        "%-24s %-22s %-24s %-26s %-24s %-24s %-16s %-16s %-16s %-16s\n",
        "Unbuffered Time (ms)",
        "Buffered Time (ms)",
        "Smart Files Time (ms)",
        "Unbuffered Memory (kb)",
        "Buffered Memory (kb)",
        "Smart Files Memory (kb)",
        "File Size (kb)",
        "Offset (kb)",
        "Insert Size (kb)",
        "Chunk Size (kb)"
    );
  }
  printf (
      "%-24f %-22f %-24f %-26f %-24f %-24f %-16f %-16f %-16f %-16f\n",
      file_unbuffered_result,
      file_buffered_result,
      ns_result,
      (fsize - ofst + insize) * 0.001,
      (chunk_size + insize) * 0.001,
      insize * 0.001,
      fsize * 0.001,
      ofst * 0.001,
      insize * 0.001,
      chunk_size * 0.001
  );

  return 0;
}
