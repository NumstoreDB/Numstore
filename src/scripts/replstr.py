import os

ROOTS = ["src/nscore", "src/numstore", "src/smartfiles"]

#OLD, NEW = "alloc.h", "core/alloc.h"
#OLD, NEW = "collections.h", "core/collections.h"
OLD, NEW = "concurrency.h", "core/concurrency.h"
OLD, NEW = "csx_assert.h", "core/csx_assert.h"
OLD, NEW = "error.h", "core/error.h"
OLD, NEW = "htable.h", "core/htable.h"
OLD, NEW = "utils.h", "core/utils.h"
OLD, NEW = "platform.h", "core/platform.h"
OLD, NEW = "stdtypes.h", "core/stdtypes.h"
OLD, NEW = "serial.h", "core/serial.h"


#OLD, NEW = "compiler.h", "nscore/compiler.h"
#OLD, NEW = "dirty_page_table.h", "nscore/dirty_page_table.h"
#OLD, NEW = "file_pager.h", "nscore/file_pager.h"
#OLD, NEW = "lock_table.h", "nscore/lock_table.h"
#OLD, NEW = "mem_vhmap.h", "nscore/mem_vhmap.h"
#OLD, NEW = "node_updates.h", "nscore/node_updates.h"
#OLD, NEW = "nsdb.h", "nscore/nsdb.h"
#OLD, NEW = "page.h", "nscore/page.h"
#OLD, NEW = "page_fixture.h", "nscore/page_fixture.h"
#OLD, NEW = "page_h.h", "nscore/page_h.h"
#OLD, NEW = "pager.h", "nscore/pager.h"
#OLD, NEW = "query.h", "nscore/query.h"
#OLD, NEW = "rope_algorithms.h", "nscore/rope_algorithms.h"
#OLD, NEW = "txn_table.h", "nscore/txn_table.h"
#OLD, NEW = "types.h", "nscore/types.h"
#OLD, NEW = "var_algorithms.h", "nscore/var_algorithms.h"
#OLD, NEW = "variables.h", "nscore/variables.h"
#OLD, NEW = "wal.h", "nscore/wal.h"

#OLD, NEW = "smfile_test_fixture.h", "smartfiles/smfile_test_fixture.h"

#OLD, NEW = "numstore.h", "compile_config.h"

OLD, NEW = "intnscore/types.h", "inttypes.h"


for ROOT in ROOTS:
    for name in os.listdir(ROOT):
        path = os.path.join(ROOT, name)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "r", encoding="utf-8") as f:
                text = f.read()
        except (UnicodeDecodeError, PermissionError):
            continue
        if OLD in text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text.replace(OLD, NEW))
            print(f"updated: {path}")
