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

#ifndef WAL_H
#define WAL_H

#include "collections.h"    // cbuffer
#include "compile_config.h" // lsn
#include "page_h.h"         // page_h
#include "pages/fsm_page.h" // fsm_page
#include "txn_table.h"      // txn

/******************************************************************************
 * SECTION: Wal Input Stream
 * ----------------------------------------------------------------------------
 * @brief Reads in a WAL
 ******************************************************************************/

struct wal_istream;

struct wal_istream *walis_open (const char *fname, error *e);
err_t               walis_close (struct wal_istream *w, error *e);
err_t               walis_crash (struct wal_istream *w, error *e);

/**
 * Seeks to a certain position on disk
 *
 * If the position is out of bounds, it
 * returns an error
 */
err_t walis_seek (struct wal_istream *w, lsn pos, error *e);

/**
 * This is the main read method
 */
err_t walis_read_all (
    struct wal_istream *w,
    bool *iseof, // At the end, this is set to true or false if we encountered
                 // the eof
    lsn   *rlsn, // NULLABLE The lsn that we just read
    u32   *checksum, // NULLABLE If passed, aggregates the checksum on [data]
    void  *dest,     // The data to read into
    u32    len,      // Length of the data to read
    error *e
);

/**
 * Because the reader can read sections of a log, and not
 * a whole log, these methods are provided to keep track of the
 * current lsn (for populating [rlsn] in walis_read_all
 *
 * When reading a _full_ log, you'd typically do:
 *
 * walis_mark_start_log()
 *
 * // read part 1
 * // read part 2
 * // read part 3
 * ...
 * // done reading this log
 *
 * walis_mark_end_log()
 */
void walis_mark_start_log (struct wal_istream *w);
void walis_mark_end_log (struct wal_istream *w);

/******************************************************************************
 * SECTION: Wal Output Stream
 * ----------------------------------------------------------------------------
 * @brief Writes out a wal
 ******************************************************************************/

struct wal_ostream
{
  i_file fd;
  latch  l;
  lsn    flushed_lsn;

  struct cbuffer buffer;
  u8             _buffer[WAL_BUFFER_CAP];
};

// Lifecycle
struct wal_ostream *walos_open (const char *fname, error *e);
err_t               walos_close (struct wal_ostream *w, error *e);

// Flush
err_t walos_flush_to (struct wal_ostream *w, lsn l, error *e);
err_t walos_flush_all (struct wal_ostream *w, error *e);

// Write
err_t walos_write_all (
    struct wal_ostream *w,
    u32                *checksum,
    const void         *data,
    u32                 len,
    error              *e
);
lsn  walos_get_next_lsn (struct wal_ostream *w);
slsn walos_truncate (struct wal_ostream *w, error *e);

err_t walos_crash (struct wal_ostream *w, error *e);

/******************************************************************************
 * SECTION: WAL Records
 ******************************************************************************/

struct wal_update_read
{
  enum wal_update_type
  {
    WUP_PHYSICAL = 1,
    WUP_FSM      = 2,
    WUP_FEXT     = 3,
  } type;

  txid tid;  // Transaction id this log is associated with
  lsn  prev; // Previous lsn

  union {
    struct physical_read_update
    {
      pgno pg;
      u8   undo[NS_PAGE_SIZE];
      u8   redo[NS_PAGE_SIZE];
    } phys;

    struct fsm_update
    {
      pgno   pg;
      p_size bit;
      u8     undo;
      u8     redo;
    } fsm;

    struct file_ext
    {
      pgno undo;
      pgno redo;
    } fext;
  };
};

struct wal_update_write
{
  enum wal_update_type type;

  txid tid;
  lsn  prev;

  union {
    struct physical_write_update
    {
      pgno pg;
      u8  *undo;
      u8  *redo;
    } phys;

    struct fsm_update fsm;

    struct file_ext fext;
  };
};

struct wal_begin
{
  txid tid;
};

struct wal_commit
{
  txid tid;
  lsn  prev;
};

struct wal_end
{
  txid tid;
  lsn  prev;
};

struct wal_clr_read
{
  enum wal_clr_type
  {
    WCLR_PHYSICAL = 1,
    WCLR_FSM      = 2,
    WCLR_DUMMY    = 3,
  } type;

  txid tid;
  lsn  prev;
  pgno pg;
  lsn  undo_next;

  union {
    struct physical_read_clr
    {
      pgno pg;
      u8   redo[NS_PAGE_SIZE];
    } phys;

    struct fsm_clr
    {
      pgno   pg;
      p_size bit;
      u8     redo;
    } fsm;
  };
};

struct wal_clr_write
{
  enum wal_clr_type type;

  txid tid;
  lsn  prev;
  lsn  undo_next;

  union {
    struct physical_write_clr
    {
      pgno pg;
      u8  *redo;
    } phys;

    struct fsm_clr fsm;
  };
};

enum wal_rec_hdr_type
{
  WL_BEGIN  = 1,
  WL_COMMIT = 2,
  WL_END    = 3,
  WL_UPDATE = 4,
  WL_CLR    = 5,
  WL_EOF    = 6,
};

struct wal_rec_hdr_read
{
  enum wal_rec_hdr_type type;

  union {
    struct wal_update_read update;
    struct wal_begin       begin;
    struct wal_commit      commit;
    struct wal_end         end;
    struct wal_clr_read    clr;
  };
};

struct wal_rec_hdr_write
{
  enum wal_rec_hdr_type type;

  union {
    struct wal_update_write update;
    struct wal_begin        begin;
    struct wal_commit       commit;
    struct wal_end          end;
    struct wal_clr_write    clr;
  };
};

void        wal_rec_hdr_read_random (struct wal_rec_hdr_read *dest);
const char *wal_rec_hdr_type_tostr (enum wal_rec_hdr_type type);
struct wal_rec_hdr_write wrhw_from_wrhr (struct wal_rec_hdr_read *src);

// Size of BEGIN entry
#define WL_BEGIN_LEN \
  (sizeof (wlh) +  /* header */   \
   sizeof (txid) + /* txid */     \
   sizeof (u32)    /* checksum */ \
  )

// Size of COMMIT entry
#define WL_COMMIT_LEN \
  (sizeof (wlh) +  /* header */   \
   sizeof (txid) + /* txid */     \
   sizeof (lsn) +  /* prev */     \
   sizeof (u32)    /* checksum */ \
  )

// Size of END entry
#define WL_END_LEN \
  (sizeof (wlh) +  /* header */   \
   sizeof (txid) + /* txid */     \
   sizeof (lsn) +  /* prev */     \
   sizeof (u32)    /* checksum */ \
  )

// Size of physical UPDATE entry
#define WL_UPDATE_LEN \
  (2 * sizeof (wlh) + /* header */   \
   sizeof (txid) +    /* txid */     \
   sizeof (lsn) +     /* prev */     \
   sizeof (pgno) +    /* pg */       \
   NS_PAGE_SIZE +        /* undo */     \
   NS_PAGE_SIZE +        /* redo */     \
   sizeof (u32)       /* checksum */ \
  )

// Size of FSM UPDATE entry
#define WL_FSM_UPDATE_LEN \
  (2 * sizeof (wlh) + /* header */   \
   sizeof (txid) +    /* txid */     \
   sizeof (lsn) +     /* prev */     \
   sizeof (pgno) +    /* pg */       \
   sizeof (p_size) +  /* bit */      \
   sizeof (u8) +      /* undo */     \
   sizeof (u8) +      /* redo */     \
   sizeof (u32)       /* checksum */ \
  )

// Size of FILE EXTENT UPDATE entry
#define WL_FILE_EXT_LEN \
  (2 * sizeof (wlh) + /* header */   \
   sizeof (txid) +    /* txid */     \
   sizeof (lsn) +     /* prev */     \
   sizeof (pgno) +    /* undo */     \
   sizeof (pgno) +    /* redo */     \
   sizeof (u32)       /* checksum */ \
  )

// Size of physical CLR entry
#define WL_CLR_LEN \
  (2 * sizeof (wlh) + /* header */    \
   sizeof (txid) +    /* txid */      \
   sizeof (lsn) +     /* prev */      \
   sizeof (pgno) +    /* pg */        \
   sizeof (lsn) +     /* undo_next */ \
   NS_PAGE_SIZE +        /* redo */      \
   sizeof (u32)       /* checksum */  \
  )

// Size of FSM CLR entry
#define WL_FSM_CLR_LEN \
  (2 * sizeof (wlh) + /* header */    \
   sizeof (txid) +    /* txid */      \
   sizeof (lsn) +     /* prev */      \
   sizeof (pgno) +    /* pg */        \
   sizeof (lsn) +     /* undo_next */ \
   sizeof (p_size) +  /* bit */       \
   sizeof (u8) +      /* redo */      \
   sizeof (u32)       /* checksum */  \
  )
// Size of DUMMY_CLR entry
#define WL_DUMMY_CLR_LEN                                          \
  (2 * sizeof (wlh) + sizeof (txid) + sizeof (lsn) + sizeof (lsn) \
   + sizeof (u32))

// Utils
stxid wrh_get_tid (const struct wal_rec_hdr_read *h);
slsn  wrh_get_prev_lsn (const struct wal_rec_hdr_read *h);
bool  wrh_is_undoable (const struct wal_rec_hdr_read *h);
bool  wrh_is_redoable (const struct wal_rec_hdr_read *h);
pgno  wrh_get_affected_pg (const struct wal_rec_hdr_read *h);
void  i_print_wal_rec_hdr_read_light (
    int                            log_level,
    const struct wal_rec_hdr_read *w,
    lsn                            l
);
struct wal_clr_write
wrh_undo (struct wal_rec_hdr_read *h, struct txn *tx, page_h *ph);
void wrh_redo (struct wal_rec_hdr_read *h, page_h *ph);

// DECODE
void walf_decode_physical_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_UPDATE_LEN]
);
void walf_decode_fsm_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_FSM_UPDATE_LEN]
);
void walf_decode_file_extend_update (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_FILE_EXT_LEN]
);
void
walf_decode_physical_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_CLR_LEN]);
void
walf_decode_fsm_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_FSM_CLR_LEN]);
void walf_decode_dummy_clr (
    struct wal_rec_hdr_read *r,
    const u8                 buf[WL_DUMMY_CLR_LEN]
);
void walf_decode_begin (struct wal_rec_hdr_read *r, const u8 buf[WL_BEGIN_LEN]);
void
walf_decode_commit (struct wal_rec_hdr_read *r, const u8 buf[WL_COMMIT_LEN]);
void walf_decode_end (struct wal_rec_hdr_read *r, const u8 buf[WL_END_LEN]);

#ifndef NTEST
bool wal_rec_hdr_read_equal (
    const struct wal_rec_hdr_read *left,
    const struct wal_rec_hdr_read *right
);
#endif

HEADER_FUNC struct wal_update_write
wup_fsm (pgno fsmpg, struct txn *tx, p_size bit, u8 undo, u8 redo)
{
  ASSERT (fsmpg % FS_BTMP_NPGS == 0);

  return (struct wal_update_write){
      .type = WUP_FSM,
      .tid  = tx->tid,
      .prev = tx->data.last_lsn,
      .fsm  = {
          .pg   = fsmpg,
          .bit  = bit,
          .undo = undo,
          .redo = redo,
      },
  };
}

/******************************************************************************
 * SECTION: WAL
 ******************************************************************************/

enum wal_flags
{
  WAL_ISNEW = (1 << 0),
};

struct wal
{
  // The file that's open
  struct string fname;

  // Input and / or output streams
  struct wal_ostream *ostream;
  struct wal_istream *istream;

  // Headers to read and write
  struct wal_rec_hdr_read  rhdr;
  struct wal_rec_hdr_write whdr;

  int flags;
  lsn start_lsn;

  latch latch;
};

DEFINE_DBG_ASSERT (struct wal, wal, w, { ASSERT (w); })

// Lifecycle
struct wal *wal_open (const char *fname, error *e);
err_t       wal_close (struct wal *w, error *e);
err_t       wal_close_and_delete (struct wal *w, error *e);
err_t       wal_delete_and_reopen (struct wal *w, error *e);
err_t       wal_write_start_lsn (struct wal *w, lsn start_lsn, error *e);

// Getters
bool wal_isnew (const struct wal *w);
lsn  wal_start_lsn (struct wal *w);
lsn  wal_size (struct wal *w);

/**
 * Flushes the wal to a certain lsn
 *
 * under the hood this is just the same as flush all
 * with an assert that [l] has been written already.
 */
err_t wal_flush_to (const struct wal *w, lsn l, error *e);
err_t wal_flush_all (const struct wal *w, error *e);

/**
 * Reading
 *
 * read_next reads in a sequential way
 * one log at a time.
 *
 * read_entry reads in a random access way
 * be careful with this one - it has
 * high cache miss rates
 */
struct wal_rec_hdr_read *wal_read_next (struct wal *w, lsn *read_lsn, error *e);
struct wal_rec_hdr_read *wal_read_first (struct wal *w, error *e);
struct wal_rec_hdr_read *wal_read_entry (struct wal *w, lsn id, error *e);

slsn wal_write_locked (struct wal *w, error *e);
slsn wal_append_begin_log (struct wal *w, txid tid, error *e);
slsn wal_append_commit_log (struct wal *w, txid tid, lsn prev, error *e);
slsn wal_append_end_log (struct wal *w, txid tid, lsn prev, error *e);
slsn wal_append_ckpt_begin (struct wal *w, error *e);
slsn
wal_append_update_log (struct wal *w, struct wal_update_write update, error *e);
slsn wal_append_clr_log (struct wal *w, struct wal_clr_write clr, error *e);

/**
 * Just append an arbitrary log
 * this is used mostly for testing
 */
slsn
wal_append_log (struct wal *w, const struct wal_rec_hdr_write *hdr, error *e);

err_t wal_crash (struct wal *w, error *e);

#endif // WAL_H
