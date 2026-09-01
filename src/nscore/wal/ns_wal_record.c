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

#include "nscore/wal/ns_wal_record.h"

#include "core/ns_numerics.h"
#include "nscore/txn_table/ns_txn_table.h"

#ifdef TESTING
#  include "core/testing/ns_testing.h"
#endif

#include <stdio.h>
#include <string.h>

/******************************************************************************
 * SECTION: WAL Decoding
 * ----------------------------------------------------------------------------
 * @brief Quick Decoding implementations for the WAL
 ******************************************************************************/

void
walf_decode_physical_update (struct wal_rec_hdr_read *r, const u8 buf[WL_UPDATE_LEN])
{
  ASSERT (r->type == WL_UPDATE);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // PG
  memcpy (&r->update.phys.pg, buf + head, sizeof (r->update.phys.pg));
  head += sizeof (r->update.phys.pg);

  // UNDO
  memcpy (r->update.phys.undo, buf + head, NS_PAGE_SIZE);
  head += NS_PAGE_SIZE;

  // REDO
  memcpy (r->update.phys.redo, buf + head, NS_PAGE_SIZE);
}

void
walf_decode_fsm_update (struct wal_rec_hdr_read *r, const u8 buf[WL_FSM_UPDATE_LEN])
{
  ASSERT (r->type == WL_UPDATE);
  ASSERT (r->update.type == WUP_FSM);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // PG
  memcpy (&r->update.fsm.pg, buf + head, sizeof (r->update.fsm.pg));
  head += sizeof (r->update.fsm.pg);

  // BIT
  memcpy (&r->update.fsm.bit, buf + head, sizeof (r->update.fsm.bit));
  head += sizeof (r->update.fsm.bit);

  // UNDO
  memcpy (&r->update.fsm.undo, buf + head, sizeof (r->update.fsm.undo));
  head += sizeof (r->update.fsm.undo);

  // REDO
  memcpy (&r->update.fsm.redo, buf + head, sizeof (r->update.fsm.redo));
}

void
walf_decode_file_extend_update (struct wal_rec_hdr_read *r, const u8 buf[WL_FILE_EXT_LEN])
{
  ASSERT (r->type == WL_UPDATE);
  ASSERT (r->update.type == WUP_FEXT);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->update.tid, buf + head, sizeof (r->update.tid));
  head += sizeof (r->update.tid);

  // PREV
  memcpy (&r->update.prev, buf + head, sizeof (r->update.prev));
  head += sizeof (r->update.prev);

  // UNDO
  memcpy (&r->update.fext.undo, buf + head, sizeof (r->update.fext.undo));
  head += sizeof (r->update.fext.undo);

  // REDO
  memcpy (&r->update.fext.redo, buf + head, sizeof (r->update.fext.redo));
}

void
walf_decode_physical_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_CLR_LEN])
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_PHYSICAL);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // PG
  memcpy (&r->clr.phys.pg, buf + head, sizeof (r->clr.phys.pg));
  head += sizeof (r->clr.phys.pg);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);

  // REDO
  memcpy (r->clr.phys.redo, buf + head, NS_PAGE_SIZE);
}

void
walf_decode_fsm_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_FSM_CLR_LEN])
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_FSM);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // PG
  memcpy (&r->clr.fsm.pg, buf + head, sizeof (r->clr.fsm.pg));
  head += sizeof (r->clr.fsm.pg);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);

  // BIT
  memcpy (&r->clr.fsm.bit, buf + head, sizeof (r->clr.fsm.bit));
  head += sizeof (r->clr.fsm.bit);

  // REDO
  memcpy (&r->clr.fsm.redo, buf + head, sizeof (r->clr.fsm.redo));
}

void
walf_decode_dummy_clr (struct wal_rec_hdr_read *r, const u8 buf[WL_DUMMY_CLR_LEN])
{
  ASSERT (r->type == WL_CLR);
  ASSERT (r->clr.type == WCLR_DUMMY);

  u32 head = 2 * sizeof (wlh);

  // TID
  memcpy (&r->clr.tid, buf + head, sizeof (r->clr.tid));
  head += sizeof (r->clr.tid);

  // PREV
  memcpy (&r->clr.prev, buf + head, sizeof (r->clr.prev));
  head += sizeof (r->clr.prev);

  // UNDO_NEXT
  memcpy (&r->clr.undo_next, buf + head, sizeof (r->clr.undo_next));
  head += sizeof (r->clr.undo_next);
}

void
walf_decode_begin (struct wal_rec_hdr_read *r, const u8 buf[WL_BEGIN_LEN])
{
  ASSERT (r->type == WL_BEGIN);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->begin.tid, buf + head, sizeof (r->begin.tid));
}

void
walf_decode_commit (struct wal_rec_hdr_read *r, const u8 buf[WL_COMMIT_LEN])
{
  ASSERT (r->type == WL_COMMIT);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->commit.tid, buf + head, sizeof (r->commit.tid));
  head += sizeof (r->commit.tid);

  // PREV
  memcpy (&r->commit.prev, buf + head, sizeof (r->commit.prev));
}

void
walf_decode_end (struct wal_rec_hdr_read *r, const u8 buf[WL_END_LEN])
{
  ASSERT (r->type == WL_END);

  u32 head = sizeof (wlh);

  // TID
  memcpy (&r->end.tid, buf + head, sizeof (r->end.tid));
  head += sizeof (r->end.tid);

  // PREV
  memcpy (&r->end.prev, buf + head, sizeof (r->end.prev));
}

/******************************************************************************
 * SECTION: WAL Record Header
 ******************************************************************************/

void
wal_rec_hdr_read_random (struct wal_rec_hdr_read *dest)
{
  dest->type = randu32r (WL_BEGIN, WL_CLR);
  switch (dest->type) {
    case WL_BEGIN: {
      dest->begin.tid = randu32 ();
      break;
    }
    case WL_COMMIT: {
      dest->commit.tid  = randu32 ();
      dest->commit.prev = randu32 ();
      break;
    }
    case WL_END: {
      dest->end.tid  = randu32 ();
      dest->end.prev = randu32 ();
      break;
    }
    case WL_UPDATE: {
      dest->update.type = randu32r (WUP_PHYSICAL, WUP_FEXT);
      dest->update.tid  = randu32 ();
      dest->update.prev = randu32 ();
      switch (dest->update.type) {
        case WUP_PHYSICAL: {
          dest->update.phys.pg = randu32 ();
          rand_bytes (dest->update.phys.undo, NS_PAGE_SIZE);
          rand_bytes (dest->update.phys.redo, NS_PAGE_SIZE);
          break;
        }
        case WUP_FSM: {
          dest->update.fsm.pg   = randu32 ();
          dest->update.fsm.bit  = randu32 ();
          dest->update.fsm.undo = randu8 ();
          dest->update.fsm.redo = randu8 ();
          break;
        }
        case WUP_FEXT: {
          dest->update.fext.undo = randu32 ();
          dest->update.fext.redo = randu32 ();
          break;
        }
      }
      break;
    }
    case WL_CLR: {
      dest->clr.type      = randu32r (WCLR_PHYSICAL, WCLR_DUMMY);
      dest->clr.tid       = randu32 ();
      dest->clr.prev      = randu32 ();
      dest->clr.undo_next = randu32 ();
      switch (dest->clr.type) {
        case WCLR_PHYSICAL: {
          dest->clr.phys.pg = randu32 ();
          rand_bytes (dest->clr.phys.redo, NS_PAGE_SIZE);
          break;
        }
        case WCLR_FSM: {
          dest->clr.fsm.pg   = randu32 ();
          dest->clr.fsm.bit  = randu32 ();
          dest->clr.fsm.redo = randu8 ();
          break;
        }
        case WCLR_DUMMY: {
          break;
        }
      }
      break;
    }
    case WL_EOF: {
      ASSERT (false);
    }
  }
}

const char *
wal_rec_hdr_type_tostr (const enum wal_rec_hdr_type type)
{
  switch (type) {
    case WL_UPDATE: {
      return "WL_UPDATE";
    }
    case WL_CLR: {
      return "WL_CLR";
    }
    case WL_BEGIN: {
      return "WL_BEGIN";
    }
    case WL_COMMIT: {
      return "WL_COMMIT";
    }
    case WL_END: {
      return "WL_END";
    }
    case WL_EOF: {
      return "WL_EOF";
    }
  }

  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING
TEST (wal_rec_hdr_type_tostr)
{
  test_assert (wal_rec_hdr_type_tostr (WL_UPDATE) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_CLR) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_BEGIN) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_COMMIT) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_END) != NULL);
  test_assert (wal_rec_hdr_type_tostr (WL_EOF) != NULL);
}
#endif

struct wal_rec_hdr_write
wrhw_from_wrhr (struct wal_rec_hdr_read *src)
{
  switch (src->type) {
    case WL_BEGIN: {
      return (struct wal_rec_hdr_write){.type = WL_BEGIN, .begin = src->begin};
    }
    case WL_COMMIT: {
      return (struct wal_rec_hdr_write){.type = WL_COMMIT, .commit = src->commit};
    }
    case WL_END: {
      return (struct wal_rec_hdr_write){.type = WL_END, .end = src->end};
    }
    case WL_UPDATE: {
      switch (src->update.type) {
        case WUP_PHYSICAL: {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_PHYSICAL,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .phys = (struct physical_write_update){
                      .pg   = src->update.phys.pg,
                      .redo = src->update.phys.redo,
                      .undo = src->update.phys.undo,
                  },
              },
          };
        }
        case WUP_FSM: {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_FSM,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .fsm  = src->update.fsm
              },
          };
        }
        case WUP_FEXT: {
          return (struct wal_rec_hdr_write){
              .type   = WL_UPDATE,
              .update = {
                  .type = WUP_FEXT,
                  .tid  = src->update.tid,
                  .prev = src->update.prev,
                  .fext = src->update.fext
              },
          };
        }
      }
      break;
    }
    case WL_CLR: {
      switch (src->clr.type) {
        case WCLR_PHYSICAL: {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_PHYSICAL,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
                  .phys      = (struct physical_write_clr){
                      .pg   = src->clr.phys.pg,
                      .redo = src->clr.phys.redo,
                  },
              },
          };
        }
        case WCLR_FSM: {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_FSM,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
                  .fsm       = src->clr.fsm
              },
          };
        }
        case WCLR_DUMMY: {
          return (struct wal_rec_hdr_write){
              .type = WL_CLR,
              .clr  = {
                  .type      = WCLR_DUMMY,
                  .tid       = src->clr.tid,
                  .prev      = src->clr.prev,
                  .undo_next = src->clr.undo_next,
              },
          };
        }
      }
      break;
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

stxid
wrh_get_tid (const struct wal_rec_hdr_read *h)
{
  switch (h->type) {
    case WL_BEGIN: {
      return h->begin.tid;
    }
    case WL_COMMIT: {
      return h->commit.tid;
    }
    case WL_END: {
      return h->end.tid;
    }
    case WL_UPDATE: {
      return h->update.tid;
    }
    case WL_CLR: {
      return h->clr.tid;
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

slsn
wrh_get_prev_lsn (const struct wal_rec_hdr_read *h)
{
  switch (h->type) {
    case WL_BEGIN: {
      return 0;
    }
    case WL_COMMIT: {
      return h->commit.prev;
    }
    case WL_END: {
      return h->end.prev;
    }
    case WL_UPDATE: {
      return h->update.prev;
    }
    case WL_CLR: {
      return h->clr.prev;
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

bool
wrh_is_undoable (const struct wal_rec_hdr_read *h)
{
  switch (h->type) {
    case WL_BEGIN: {
      return false;
    }
    case WL_COMMIT: {
      return false;
    }
    case WL_END: {
      return false;
    }
    case WL_UPDATE: {
      switch (h->update.type) {
        case WUP_PHYSICAL: {
          return true;
        }
        case WUP_FSM: {
          return true;
        }
        case WUP_FEXT: {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR: {
      return false;
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

bool
wrh_is_redoable (const struct wal_rec_hdr_read *h)
{
  switch (h->type) {
    case WL_BEGIN: {
      return false;
    }
    case WL_COMMIT: {
      return false;
    }
    case WL_END: {
      return false;
    }
    case WL_UPDATE: {
      switch (h->update.type) {
        case WUP_PHYSICAL: {
          return true;
        }
        case WUP_FSM: {
          return true;
        }
        case WUP_FEXT: {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR: {
      switch (h->clr.type) {
        case WCLR_PHYSICAL: {
          return true;
        }
        case WCLR_FSM: {
          return true;
        }
        case WCLR_DUMMY: {
          return false;
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

pgno
wrh_get_affected_pg (const struct wal_rec_hdr_read *h)
{
  switch (h->type) {
    case WL_BEGIN: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE: {
      switch (h->update.type) {
        case WUP_PHYSICAL: {
          return h->update.phys.pg;
        }
        case WUP_FSM: {
          return h->update.fsm.pg;
        }
        case WUP_FEXT: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR: {
      switch (h->clr.type) {
        case WCLR_PHYSICAL: {
          return h->clr.phys.pg;
        }
        case WCLR_FSM: {
          return h->clr.fsm.pg;
        }
        case WCLR_DUMMY: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

#ifdef TESTING
bool
wal_rec_hdr_read_equal (const struct wal_rec_hdr_read *left, const struct wal_rec_hdr_read *right)
{
  if (left->type != right->type) {
    return false;
  }

  bool match = true;

  switch (left->type) {
    case WL_UPDATE: {
      if (left->update.type != right->update.type) {
        return false;
      }

      match = match && left->update.tid == right->update.tid;
      match = match && left->update.prev == right->update.prev;

      switch (left->update.type) {
        case WUP_FSM: {
          match = match && left->update.fsm.pg == right->update.fsm.pg;
          match = match && left->update.fsm.undo == right->update.fsm.undo;
          match = match && left->update.fsm.redo == right->update.fsm.redo;
          break;
        }
        case WUP_PHYSICAL: {
          match = match && left->update.phys.pg == right->update.phys.pg;
          match = match
                  && memcmp (left->update.phys.undo, right->update.phys.undo, NS_PAGE_SIZE) == 0;
          match = match
                  && memcmp (left->update.phys.redo, right->update.phys.redo, NS_PAGE_SIZE) == 0;
          break;
        }
        case WUP_FEXT: {
          match = match && left->update.fext.undo == right->update.fext.undo;
          match = match && left->update.fext.redo == right->update.fext.redo;
          break;
        }
      }
      break;
    }

    case WL_CLR: {
      if (left->clr.type != right->clr.type) {
        return false;
      }

      match = match && left->clr.tid == right->clr.tid;
      match = match && left->clr.prev == right->clr.prev;
      match = match && left->clr.undo_next == right->clr.undo_next;

      switch (left->clr.type) {
        case WCLR_PHYSICAL: {
          match = match && left->clr.phys.pg == right->clr.phys.pg;
          match = match && memcmp (left->clr.phys.redo, right->clr.phys.redo, NS_PAGE_SIZE) == 0;
          break;
        }
        case WCLR_FSM: {
          match = match && left->clr.fsm.pg == right->clr.fsm.pg;
          match = match && left->clr.fsm.redo == right->clr.fsm.redo;
          break;
        }
        case WCLR_DUMMY: {
          break;
        }
      }

      break;
    }

    case WL_BEGIN: {
      match = match && left->begin.tid == right->begin.tid;
      break;
    }

    case WL_END: {
      match = match && left->end.tid == right->end.tid;
      match = match && left->end.prev == right->end.prev;
      break;
    }

    case WL_COMMIT: {
      match = match && left->commit.tid == right->commit.tid;
      match = match && left->commit.prev == right->commit.prev;
      break;
    }

    case WL_EOF: {
      return true;
    }
  }

  return match;
}
#endif

void
i_print_wal_rec_hdr_read_light (const int log_level, const struct wal_rec_hdr_read *r, const lsn l)
{
  char        fields[128];
  const char *name = "?";
  const lsn  *prev = NULL;

  (void)l;    // Unused
  (void)name; // Unused

  switch (r->type) {
    case WL_UPDATE:
      switch (r->update.type) {
        case WUP_PHYSICAL: {
          name = "UPDATE PHYS";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno,
              r->update.tid,
              r->update.phys.pg
          );
          prev = &r->update.prev;
          break;
        }
        case WUP_FSM: {
          name = "UPDATE FSM";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno ", undo = 0x%02x, redo = 0x%02x",
              r->update.tid,
              r->update.fsm.pg,
              (unsigned)r->update.fsm.undo,
              (unsigned)r->update.fsm.redo
          );
          prev = &r->update.prev;
          break;
        }
        case WUP_FEXT: {
          name = "UPDATE FEXT";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", undo_pgs = %8" PRpgno ", redo_pgs = %8" PRpgno,
              r->update.tid,
              r->update.fext.undo,
              r->update.fext.redo
          );
          prev = &r->update.prev;
          break;
        }
      }
      break;

    case WL_CLR:
      switch (r->clr.type) {
        case WCLR_PHYSICAL: {
          name = "CLR PHYS";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno ", undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.phys.pg,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
        case WCLR_FSM: {
          name = "CLR FSM";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", pg   = %8" PRpgno ", redo = 0x%02x, undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.fsm.pg,
              (unsigned)r->clr.fsm.redo,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
        case WCLR_DUMMY: {
          name = "CLR DUMMY";
          snprintf (
              fields,
              sizeof fields,
              "txid = %8" PRtxid ", undoNxt = %15" PRlsn,
              r->clr.tid,
              r->clr.undo_next
          );
          prev = &r->clr.prev;
          break;
        }
      }
      break;

    case WL_BEGIN: {
      name = "BEGIN";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->begin.tid);
      break;
    }

    case WL_COMMIT: {
      name = "COMMIT";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->commit.tid);
      prev = &r->commit.prev;
      break;
    }

    case WL_END: {
      name = "END";
      snprintf (fields, sizeof fields, "txid = %8" PRtxid, r->end.tid);
      prev = &r->end.prev;
      break;
    }

    case WL_EOF: {
      i_log_printf (log_level, "%15" PRlsn "  WL_EOF\n", l);
      return;
    }
  }

  /* Widths set in one place:
       11 = strlen("UPDATE FEXT")  -- widest type name
       72 = widest fields line     -- "CLR FSM" case
     Bump them if a new record type pushes past these. */
  if (prev) {
    i_log_printf (
        log_level,
        "%15" PRlsn "  %-11s  [ %-72s ] --> %" PRlsn "\n",
        l,
        name,
        fields,
        *prev
    );
  } else {
    i_log_printf (log_level, "%15" PRlsn "  %-11s  [ %-72s ]\n", l, name, fields);
  }
}

struct wal_clr_write
wrh_undo (struct wal_rec_hdr_read *h, struct ns_txn *tx, page_h *ph)
{
  switch (h->type) {
    case WL_BEGIN: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE: {
      switch (h->update.type) {
        case WUP_PHYSICAL: {
          memcpy (page_h_w (ph), h->update.phys.undo, NS_PAGE_SIZE);
          return (struct wal_clr_write){
              .type      = WCLR_PHYSICAL,
              .tid       = h->update.tid,
              .prev      = tx->data.last_lsn,
              .undo_next = h->update.prev,
              .phys      = {
                  .pg   = wrh_get_affected_pg (h),
                  .redo = h->update.phys.undo,
              },
          };
        }
        case WUP_FSM: {
          if (h->update.fsm.undo) {
            fsm_set_bit (page_h_w (ph), h->update.fsm.bit);
          } else {
            fsm_clr_bit (page_h_w (ph), h->update.fsm.bit);
          }
          return (struct wal_clr_write){
              .type      = WCLR_FSM,
              .tid       = h->update.tid,
              .prev      = tx->data.last_lsn,
              .undo_next = h->update.prev,
              .fsm       = {
                  .pg   = page_h_pgno (ph),
                  .bit  = h->update.fsm.bit,
                  .redo = h->update.fsm.undo,
              },
          };
        }
        case WUP_FEXT: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR: {
      switch (h->clr.type) {
        case WCLR_PHYSICAL: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
        case WCLR_FSM: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
        case WCLR_DUMMY: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}

void
wrh_redo (struct wal_rec_hdr_read *h, page_h *ph)
{
  switch (h->type) {
    case WL_BEGIN: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_COMMIT: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_END: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_UPDATE: {
      switch (h->update.type) {
        case WUP_PHYSICAL: {
          memcpy (page_h_w (ph)->raw, h->update.phys.redo, NS_PAGE_SIZE);
          return;
        }
        case WUP_FSM: {
          if (h->update.fsm.redo) {
            fsm_set_bit (page_h_w (ph), h->update.fsm.bit);
          } else {
            fsm_clr_bit (page_h_w (ph), h->update.fsm.bit);
          }
          return;
        }
        case WUP_FEXT: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_CLR: {
      switch (h->clr.type) {
        case WCLR_PHYSICAL: {
          memcpy (page_h_w (ph)->raw, h->clr.phys.redo, NS_PAGE_SIZE);
          return;
        }
        case WCLR_FSM: {
          if (h->clr.fsm.redo) {
            fsm_set_bit (page_h_w (ph), h->clr.fsm.bit);
          } else {
            fsm_clr_bit (page_h_w (ph), h->clr.fsm.bit);
          }
          return;
        }
        case WCLR_DUMMY: {
          UNREACHABLE (); // LCOV_EXCL_LINE
        }
      }
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
    case WL_EOF: {
      UNREACHABLE (); // LCOV_EXCL_LINE
    }
  }
  UNREACHABLE (); // LCOV_EXCL_LINE
}
