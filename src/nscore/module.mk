# Algorithms
LIBNS_SRCS += src/nscore/algorithms/ns_node_updates.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_balance_and_release.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_get_number_of_layers.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_insert.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_read.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_rebalance.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_remove.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_seek.c
LIBNS_SRCS += src/nscore/algorithms/rope/ns_rope_write.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_find_var_page.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_init_var_hash_map.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_read_var_page.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_var_create.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_var_delete.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_var_get.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_var_get_or_create.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_var_update.c
LIBNS_SRCS += src/nscore/algorithms/var/ns_write_var_page.c

# Compiler
LIBNS_SRCS += src/nscore/compiler/ns_lexer.c
LIBNS_SRCS += src/nscore/compiler/ns_tokens.c

# Parsers
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_multi_user_stride.c
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_query.c
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_subtype.c
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_type.c
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_type_ref.c
LIBNS_SRCS += src/nscore/compiler/parsers/ns_parse_user_stride.c

# Dirty Page Table
LIBNS_SRCS += src/nscore/dpg_table/ns_dirty_page_table.c

# Lock Table
LIBNS_SRCS += src/nscore/lock_table/ns_lock_table.c

# NSDB
LIBNS_SRCS += src/nscore/nsdb/ns_nsdb.c
LIBNS_SRCS += src/nscore/nsdb/ns_nsdb_cli.c
LIBNS_SRCS += src/nscore/nsdb/ns_nsdb_execute.c

# Page
LIBNS_SRCS += src/nscore/page/ns_page.c
LIBNS_SRCS += src/nscore/page/ns_page_data_list.c
LIBNS_SRCS += src/nscore/page/ns_page_fsm.c
LIBNS_SRCS += src/nscore/page/ns_page_inner_node.c
LIBNS_SRCS += src/nscore/page/ns_page_var_hash_page.c
LIBNS_SRCS += src/nscore/page/ns_page_var_page.c
LIBNS_SRCS += src/nscore/page/ns_page_var_tail.c

# Pager
LIBNS_SRCS += src/nscore/disk_pager/ns_file_pager.c
LIBNS_SRCS += src/nscore/pager/ns_pager.c

# Testing
LIBNS_SRCS += src/nscore/testing/ns_page_fixture.c

# Txn Table
LIBNS_SRCS += src/nscore/txn_table/ns_txn_table.c

# Types
LIBNS_SRCS += src/nscore/types/ns_kvt.c
LIBNS_SRCS += src/nscore/types/ns_query.c
LIBNS_SRCS += src/nscore/types/ns_sarray_t.c
LIBNS_SRCS += src/nscore/types/ns_struct_t.c
LIBNS_SRCS += src/nscore/types/ns_subtype.c
LIBNS_SRCS += src/nscore/types/ns_type_accessor.c
LIBNS_SRCS += src/nscore/types/ns_type_ref.c
LIBNS_SRCS += src/nscore/types/ns_types.c
LIBNS_SRCS += src/nscore/types/ns_union_t.c
LIBNS_SRCS += src/nscore/types/ns_variables.c

# WAL
LIBNS_SRCS += src/nscore/wal/ns_wal.c
LIBNS_SRCS += src/nscore/wal/ns_wal_istream.c
LIBNS_SRCS += src/nscore/wal/ns_wal_ostream.c
LIBNS_SRCS += src/nscore/wal/ns_wal_record.c

############ Bins

$(BIN_DIR)/nspprint: src/nscore/pager/tools/ns_nspprint.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/nsspprint: src/nscore/pager/tools/ns_nsspprint.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/dlread: src/nscore/pager/tools/ns_dlread.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/walpprint: src/nscore/wal/tools/ns_walpprint.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/print_query: src/nscore/compiler/tools/print_query.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/print_type: src/nscore/compiler/tools/print_type.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/resolve_type_ref: src/nscore/compiler/tools/resolve_type_ref.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL += $(BIN_DIR)/nspprint
ALL += $(BIN_DIR)/nsspprint
ALL += $(BIN_DIR)/walpprint
ALL += $(BIN_DIR)/dlread
ALL += $(BIN_DIR)/print_query
ALL += $(BIN_DIR)/print_type
ALL += $(BIN_DIR)/resolve_type_ref
