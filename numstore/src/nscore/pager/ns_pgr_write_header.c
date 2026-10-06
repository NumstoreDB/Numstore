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

#include "core/ns_error.h"
#include "nscore/disk_pager/ns_file_pager.h"
#include "nscore/page/ns_page_fixture.h"
#include "nscore/pager/ns_pager.h"

err_t
pgr_write_lsn0 (struct pager *p, lsn lsn0, error *e)
{
  p->header.lsn0    = lsn0;
  p->header.lsn0csm = checksum_init ();
  checksum_execute (&p->header.lsn0csm, (void *)&lsn0, sizeof (lsn));

  memcpy (p->_header + LSN0_OFST, &p->header.lsn0, sizeof (lsn));
  memcpy (p->_header + LSN0_CSM_OFST, &p->header.lsn0csm, sizeof (u32));

  return fpgr_write_header (p->fp, p->_header, LSN0_OFST, sizeof (lsn) + sizeof (u32), e);
}

err_t
pgr_write_lsn1 (struct pager *p, lsn lsn1, error *e)
{
  p->header.lsn1    = lsn1;
  p->header.lsn1csm = checksum_init ();
  checksum_execute (&p->header.lsn1csm, (void *)&lsn1, sizeof (lsn));

  memcpy (p->_header + LSN1_OFST, &p->header.lsn1, sizeof (lsn));
  memcpy (p->_header + LSN1_CSM_OFST, &p->header.lsn1csm, sizeof (u32));

  return fpgr_write_header (
      p->fp,
      p->_header + LSN1_OFST,
      LSN1_OFST,
      sizeof (lsn) + sizeof (u32),
      e
  );
}

err_t
pgr_write_next_lsn (struct pager *p, lsn l, error *e)
{
  if (p->header.lsn0 > p->header.lsn1) {
    return pgr_write_lsn1 (p, l, e);
  }
  return pgr_write_lsn0 (p, l, e);
}

err_t
pgr_write_header (struct pager *p, error *e)
{
  p->header.lsn0csm = checksum_init ();
  p->header.lsn1csm = checksum_init ();

  checksum_execute (&p->header.lsn0csm, (void *)&p->header.lsn0, sizeof (lsn));
  checksum_execute (&p->header.lsn1csm, (void *)&p->header.lsn1, sizeof (lsn));

  memcpy (p->_header + LSN0_OFST, &p->header.lsn0, sizeof (lsn));
  memcpy (p->_header + LSN0_CSM_OFST, &p->header.lsn0csm, sizeof (u32));
  memcpy (p->_header + LSN1_OFST, &p->header.lsn1, sizeof (lsn));
  memcpy (p->_header + LSN1_CSM_OFST, &p->header.lsn1csm, sizeof (u32));

  return fpgr_write_header (p->fp, p->_header, 0, PAGE_HEADER_LEN, e);
}
