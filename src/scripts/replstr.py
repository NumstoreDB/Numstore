import os

ROOTS = ["src"]

# Map of OLD -> NEW replacements
REPLACEMENTS = {
    "i_printf": "i_log_printf",
    # add more OLD: NEW pairs here
    "include \"string.h\"": "include \"core/ns_string.h\"",
    "include \"slab_alloc.h\"": "include \"core/ns_slab_alloc.h\"",
    "include \"platform.h\"" : "include \"core/ns_platform.h\"",
    "include \"alloc.h\"" : "include \"core/ns_alloc.h\"",
    "include \"numerics.h\"" : "include \"core/ns_numerics.h\"",
    "include \"logging.h\"" : "include \"core/ns_logging.h\"",
    "include \"htable.h\"" : "include \"core/ns_htable.h\"",
    "include \"serial.h\"" : "include \"core/ns_serial.h\"",
    "include \"dbl_buffer.h\"" : "include \"core/ns_dbl_buffer.h\"",
    "include \"cbuffer.h\"" : "include \"core/ns_cbuffer.h\"",
    "include \"chunk_alloc.h\"" : "include \"core/ns_chunk_alloc.h\"",
    "include \"linked_list.h\"" : "include \"core/ns_linked_list.h\"",
    "include \"data_validator.h\"" : "include \"core/ns_data_validator.h\"",
    "include \"concurrency.h\"" : "include \"core/ns_concurrency.h\"",
    "include \"bounds.h\"" : "include \"core/ns_bounds.h\"",
    "include \"unit_tests.h\"" : "include \"core/testing/ns_unit_tests.h\"",
    "include \"testing.h\"" : "include \"core/testing/ns_testing.h\"",
    "include \"stream.h\"" : "include \"core/ns_stream.h\"",
    "include \"ext_array.h\"" : "include \"core/ns_ext_array.h\"",
    "include \"byte_accessor.h\"" : "include \"core/ns_byte_accessor.h\"",
    "include \"os.h\"" : "include \"core/os/ns_os.h\"",
    "include \"error.h\"" : "include \"core/ns_error.h\"",
    "include \"utils.h\"" : "include \"core/ns_utils.h\"",
    "include \"csx_assert.h\"" : "include \"core/ns_csx_assert.h\"",
    "include \"robin_hood_ht.h\"" : "include \"core/ns_robin_hood_ht.h\"",
    "include \"block_array.h\"" : "include \"core/ns_block_array.h\"",
    "include \"data_writer.h\"" : "include \"core/ns_data_writer.h\"",
    "include \"stdtypes.h\"" : "include \"core/ns_stdtypes.h\"",
    "include \"stride.h\"" : "include \"core/ns_stride.h\"",
    "include \"smfile_test_fixture.h\"" : "include \"smartfiles/testing/ns_smfile_test_fixture.h\"",
    "include \"subtype.h\"" : "include \"numstore/types/ns_subtype.h\"",
    "include \"type_accessor.h\"" : "include \"numstore/types/ns_type_accessor.h\"",
    "include \"type_ref.h\"" : "include \"numstore/types/ns_type_ref.h\"",
    "include \"kvt.h\"" : "include \"numstore/types/ns_kvt.h\"",
    "include \"union_t.h\"" : "include \"numstore/types/ns_union_t.h\"",
    "include \"sarray_t.h\"" : "include \"numstore/types/ns_sarray_t.h\"",
    "include \"types.h\"" : "include \"numstore/types/ns_types.h\"",
    "include \"struct_t.h\"" : "include \"numstore/types/ns_struct_t.h\"",
    "include \"variables.h\"" : "include \"numstore/ns_variables.h\"",
    "include \"swarm_tests.h\"" : "include \"numstore/testing/ns_swarm_tests.h\"",
    "include \"query.h\"" : "include \"numstore/ns_query.h\"",
    "include \"lexer.h\"" : "include \"numstore/compiler/ns_lexer.h\"",
    "include \"parser.h\"" : "include \"numstore/compiler/parsers/ns_parser.h\"",
    "include \"parser.h\"" : "include \"numstore/compiler/ns_parser.h\"",
    "include \"tokens.h\"" : "include \"numstore/compiler/ns_tokens.h\"",
    "include \"compiler.h\"" : "include \"numstore/compiler/ns_compiler.h\"",
    "include \"page.h\"" : "include \"nscore/page/ns_page.h\"",
    "include \"page_inner_node.h\"" : "include \"nscore/page/ns_page_inner_node.h\"",
"include \"page_var_page.h\"" : "include \"nscore/page/ns_page_var_page.h\"",
    "include \"page_fsm.h\"" : "include \"nscore/page/ns_page_fsm.h\"",
    "include \"page_delegate.h\"" : "include \"nscore/page/ns_page_delegate.h\"",
    "include \"page_var_hash_page.h\"" : "include \"nscore/page/ns_page_var_hash_page.h\"",
    "include \"page_h.h\"" : "include \"nscore/page/ns_page_h.h\"",
    "include \"page_var_tail.h\"" : "include \"nscore/page/ns_page_var_tail.h\"",
    "include \"page_data_list.h\"" : "include \"nscore/page/ns_page_data_list.h\"",
    "include \"wal_record.h\"" : "include \"nscore/wal/ns_wal_record.h\"",
    "include \"wal_ostream.h\"" : "include \"nscore/wal/ns_wal_ostream.h\"",
    "include \"wal_istream.h\"" : "include \"nscore/wal/ns_wal_istream.h\"",
    "include \"wal.h\"" : "include \"nscore/wal/ns_wal.h\"",
    "include \"dirty_page_table.h\"" : "include \"nscore/ns_dirty_page_table.h\"",
    "include \"var_algorithms.h\"" : "include \"nscore/algorithms/ns_var_algorithms.h\"",
    "include \"rope_algorithms.h\"" : "include \"nscore/algorithms/ns_rope_algorithms.h\"",
    "include \"lock_table.h\"" : "include \"nscore/ns_lock_table.h\"",
    "include \"mem_vhmap.h\"" : "include \"nscore/ns_mem_vhmap.h\"",
    "include \"txn_table.h\"" : "include \"nscore/ns_txn_table.h\"",
    "include \"node_updates.h\"" : "include \"nscore/ns_node_updates.h\"",
    "include \"page_h.h\"" : "include \"nscore/ns_page_h.h\"",
    "include \"page_fixture.h\"" : "include \"nscore/ns_page_fixture.h\"",
    "include \"nsdb.h\"" : "include \"nscore/ns_nsdb.h\"",
    "include \"pager.h\"" : "include \"nscore/pager/ns_pager.h\"",
    "include \"file_pager.h\"" : "include \"nscore/pager/ns_file_pager.h\"",
}

for ROOT in ROOTS:
    for dirpath, _, filenames in os.walk(ROOT):
        for name in filenames:
            path = os.path.join(dirpath, name)
            try:
                with open(path, "r", encoding="utf-8") as f:
                    text = f.read()
            except (UnicodeDecodeError, PermissionError):
                continue

            new_text = text
            hits = []
            for old, new in REPLACEMENTS.items():
                if old in new_text:
                    hits.append(old)
                    new_text = new_text.replace(old, new)

            if hits:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(new_text)
                print(f"updated: {path} ({', '.join(hits)})")
