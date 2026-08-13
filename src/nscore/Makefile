C_SRCS := 

# Algorithms
C_SRCS += algorithms/ns_balance_and_release.c
C_SRCS += algorithms/ns_find_var_page.c
C_SRCS += algorithms/ns_init_var_hash_map.c
C_SRCS += algorithms/ns_read_var_page.c
C_SRCS += algorithms/ns_rope_get_number_of_layers.c
C_SRCS += algorithms/ns_rope_insert.c
C_SRCS += algorithms/ns_rope_read.c
C_SRCS += algorithms/ns_rope_rebalance.c
C_SRCS += algorithms/ns_rope_remove.c
C_SRCS += algorithms/ns_rope_seek.c
C_SRCS += algorithms/ns_rope_write.c
C_SRCS += algorithms/ns_var_create.c
C_SRCS += algorithms/ns_var_delete.c
C_SRCS += algorithms/ns_var_get.c
C_SRCS += algorithms/ns_var_get_or_create.c
C_SRCS += algorithms/ns_var_update.c
C_SRCS += algorithms/ns_write_var_page.c

# Parsers
C_SRCS += compiler/parsers/ns_parse_multi_user_stride.c
C_SRCS += compiler/parsers/ns_parse_query.c
C_SRCS += compiler/parsers/ns_parse_subtype.c
C_SRCS += compiler/parsers/ns_parse_type.c
C_SRCS += compiler/parsers/ns_parse_type_ref.c
C_SRCS += compiler/parsers/ns_parse_user_stride.c

# Compiler
C_SRCS += compiler/ns_lexer.c
C_SRCS += compiler/ns_tokens.c

# Page
C_SRCS += page/ns_page.c
C_SRCS += page/ns_page_data_list.c
C_SRCS += page/ns_page_fsm.c
C_SRCS += page/ns_page_inner_node.c
C_SRCS += page/ns_page_var_hash_page.c
C_SRCS += page/ns_page_var_page.c
C_SRCS += page/ns_page_var_tail.c

# Pager
C_SRCS += pager/ns_file_pager.c
C_SRCS += pager/ns_pager.c

# Types
C_SRCS += types/ns_kvt.c
C_SRCS += types/ns_query.c
C_SRCS += types/ns_sarray_t.c
C_SRCS += types/ns_struct_t.c
C_SRCS += types/ns_subtype.c
C_SRCS += types/ns_type_accessor.c
C_SRCS += types/ns_type_ref.c
C_SRCS += types/ns_types.c
C_SRCS += types/ns_union_t.c

# WAL
C_SRCS += wal/ns_wal.c
C_SRCS += wal/ns_wal_istream.c
C_SRCS += wal/ns_wal_ostream.c
C_SRCS += wal/ns_wal_record.c
C_SRCS += ns_dirty_page_table.c
C_SRCS += ns_lock_table.c
C_SRCS += ns_mem_vhmap.c
C_SRCS += ns_node_updates.c
C_SRCS += ns_nsdb.c
C_SRCS += ns_page_fixture.c
C_SRCS += ns_txn_table.c
C_SRCS += ns_variables.c

C_OBJS := $(C_SRCS:.c=.o)

LIB_TARGET := $(LIB_DIR)/libnscore.a

.PHONY: all clean
all: $(LIB_TARGET)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJS) $(LIB_TARGET)
