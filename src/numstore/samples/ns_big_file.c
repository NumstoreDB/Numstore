/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
#include "numstore.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_B_SIZE 900000000
#define DEFAULT_I_SIZE 1000

static double
elapsed_sec (struct timespec start, struct timespec end)
{
  return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int
main (int argc, char **argv)
{
  size_t          b_size       = argc > 1 ? strtoull (argv[1], NULL, 10) : DEFAULT_B_SIZE;
  size_t          i_size       = argc > 2 ? strtoull (argv[2], NULL, 10) : DEFAULT_I_SIZE;

  uint8_t        *backing_data = malloc (b_size);
  uint8_t        *insert_data  = malloc (i_size);
  struct timespec start, end;
  double          ns_result, naive_result;
  {
    // Set up - build the backing file first
    nsdb_cleanup ("sample_big_file");
    nsdb_t *ns = nsdb_open ("sample_big_file");
    nsdb_fexecute (ns, NULL, "create example u8", NULL);

    // Do one big insert at offset 0
    nsdb_fexecute (ns, NULL, "insert example 0 %ld", backing_data, b_size);

    // Timed Section
    //    Inner insert is first class
    {
      clock_gettime (CLOCK_MONOTONIC, &start);
      nsdb_fexecute (ns, NULL, "insert example 10 %ld", insert_data, i_size);
      clock_gettime (CLOCK_MONOTONIC, &end);
      ns_result = elapsed_sec (start, end);
    }

    nsdb_close (ns);
  }

  {
    // Set up - build the backing file first
    remove ("sample_naive_file");
    int fd = open ("sample_naive_file", O_RDWR | O_CREAT | O_TRUNC, 0644);

    // Do one big write of the backing data
    write (fd, backing_data, b_size);

    size_t   tail_size = b_size - 10;
    uint8_t *tail      = malloc (tail_size);

    // Timed Section
    //    Read the tail
    //    Write the new data
    //    Write the tail
    {
      clock_gettime (CLOCK_MONOTONIC, &start);
      pread (fd, tail, tail_size, 10);
      pwrite (fd, tail, tail_size, 10 + i_size);
      pwrite (fd, insert_data, i_size, 10);
      clock_gettime (CLOCK_MONOTONIC, &end);
      naive_result = elapsed_sec (start, end);
    }

    close (fd);
    free (tail);
  }
  free (backing_data);
  free (insert_data);
  printf ("nsdb insert:  %f seconds\n", ns_result);
  printf ("naive insert: %f seconds\n", naive_result);
  return 0;
}
