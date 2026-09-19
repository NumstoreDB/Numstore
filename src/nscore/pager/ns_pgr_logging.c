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

#include "core/testing/ns_testing.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

void
i_log_page_table (const int log_level, bool only_present, struct pager *p)
{
  DBG_ASSERT (pager, p);

  for (u32 i = 0; i < MEMORY_PAGE_LEN; ++i) {
    const struct page_frame *mp = &p->pages[i];
    if (mp->flags & PW_PRESENT) {
      i_log_printf (
          log_level,
          "%u |(PAGE)    pg: %" PRpgno
          " pin: %d acess: %d dirty: %d present: %d "
          "sibling: %d type: %d data: %d ctrl: %d|\n",
          i,
          mp->page.pg,
          mp->pin,
          (mp->flags & PW_ACCESS) != 0,
          dpgt_exists (p->dpt, mp->page.pg),
          (mp->flags & PW_PRESENT) != 0,
          mp->wsibling,
          page_get_type (&mp->page),
          mp->data,
          mp->ctrl
      );
    } else if (!only_present) {
      i_log_printf (log_level, "%u | |\n", i);
    }
  }
}

#ifdef TESTING
TEST (i_log_page_table)
{
  struct pgr_fixture f;
  pgr_fixture_create (&f);

  page_h a = page_h_create ();
  pgr_new (&a, f.p, &f.tx, PG_DATA_LIST, &f.e);
  dl_make_valid (page_h_w (&a));
  pgr_release (f.p, &a, PG_DATA_LIST, &f.e);

  i_log_page_table (LOG_INFO, false, f.p);

  pgr_fixture_teardown (&f);
}
#endif
