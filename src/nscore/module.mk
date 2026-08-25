# Algorithms
ALL_SRCS += src/nscore/algorithms/ns_node_updates.c
ALL_SRCS += src/nscore/algorithms/rope/ns_balance_and_release.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_get_number_of_layers.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_insert.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_read.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_rebalance.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_remove.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_seek.c
ALL_SRCS += src/nscore/algorithms/rope/ns_rope_write.c
ALL_SRCS += src/nscore/algorithms/var/ns_find_var_page.c
ALL_SRCS += src/nscore/algorithms/var/ns_init_var_hash_map.c
ALL_SRCS += src/nscore/algorithms/var/ns_read_var_page.c
ALL_SRCS += src/nscore/algorithms/var/ns_var_create.c
ALL_SRCS += src/nscore/algorithms/var/ns_var_delete.c
ALL_SRCS += src/nscore/algorithms/var/ns_var_get.c
ALL_SRCS += src/nscore/algorithms/var/ns_var_get_or_create.c
ALL_SRCS += src/nscore/algorithms/var/ns_var_update.c
ALL_SRCS += src/nscore/algorithms/var/ns_write_var_page.c

# Compiler
ALL_SRCS += src/nscore/compiler/ns_lexer.c
ALL_SRCS += src/nscore/compiler/ns_tokens.c

# Parsers
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_multi_user_stride.c
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_query.c
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_subtype.c
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_type.c
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_type_ref.c
ALL_SRCS += src/nscore/compiler/parsers/ns_parse_user_stride.c

# Dirty Page Table
ALL_SRCS += src/nscore/dpg_table/ns_dirty_page_table.c

# Lock Table
ALL_SRCS += src/nscore/lock_table/ns_lock_table.c

# NSDB
ALL_SRCS += src/nscore/nsdb/ns_nsdb.c

# Page
ALL_SRCS += src/nscore/page/ns_page.c
ALL_SRCS += src/nscore/page/ns_page_data_list.c
ALL_SRCS += src/nscore/page/ns_page_fsm.c
ALL_SRCS += src/nscore/page/ns_page_inner_node.c
ALL_SRCS += src/nscore/page/ns_page_var_hash_page.c
ALL_SRCS += src/nscore/page/ns_page_var_page.c
ALL_SRCS += src/nscore/page/ns_page_var_tail.c

# Pager
ALL_SRCS += src/nscore/disk_pager/ns_file_pager.c
ALL_SRCS += src/nscore/pager/ns_pager.c

# Testing
ALL_SRCS += src/nscore/testing/ns_page_fixture.c

# Txn Table
ALL_SRCS += src/nscore/txn_table/ns_txn_table.c

# Types
ALL_SRCS += src/nscore/types/ns_kvt.c
ALL_SRCS += src/nscore/types/ns_query.c
ALL_SRCS += src/nscore/types/ns_sarray_t.c
ALL_SRCS += src/nscore/types/ns_struct_t.c
ALL_SRCS += src/nscore/types/ns_subtype.c
ALL_SRCS += src/nscore/types/ns_type_accessor.c
ALL_SRCS += src/nscore/types/ns_type_ref.c
ALL_SRCS += src/nscore/types/ns_types.c
ALL_SRCS += src/nscore/types/ns_union_t.c
ALL_SRCS += src/nscore/types/ns_variables.c

# WAL
ALL_SRCS += src/nscore/wal/ns_wal.c
ALL_SRCS += src/nscore/wal/ns_wal_istream.c
ALL_SRCS += src/nscore/wal/ns_wal_ostream.c
ALL_SRCS += src/nscore/wal/ns_wal_record.c

############ Bins

$(BIN_DIR)/nspprint: src/nscore/pager/tools/ns_nspprint.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/nsspprint: src/nscore/pager/tools/ns_nsspprint.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/dlread: src/nscore/pager/tools/ns_dlread.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/walpprint: src/nscore/wal/tools/ns_walpprint.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/print_query: src/nscore/compiler/tools/print_query.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/print_type: src/nscore/compiler/tools/print_type.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/resolve_type_ref: src/nscore/compiler/tools/resolve_type_ref.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL_BINS += $(BIN_DIR)/nspprint
ALL_BINS += $(BIN_DIR)/nsspprint
ALL_BINS += $(BIN_DIR)/walpprint
ALL_BINS += $(BIN_DIR)/dlread
ALL_BINS += $(BIN_DIR)/print_query
ALL_BINS += $(BIN_DIR)/print_type
ALL_BINS += $(BIN_DIR)/resolve_type_ref
