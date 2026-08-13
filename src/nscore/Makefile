SRCS := 

# Algorithms
SRCS += algorithms/ns_balance_and_release.c
SRCS += algorithms/ns_find_var_page.c
SRCS += algorithms/ns_init_var_hash_map.c
SRCS += algorithms/ns_read_var_page.c
SRCS += algorithms/ns_rope_get_number_of_layers.c
SRCS += algorithms/ns_rope_insert.c
SRCS += algorithms/ns_rope_read.c
SRCS += algorithms/ns_rope_rebalance.c
SRCS += algorithms/ns_rope_remove.c
SRCS += algorithms/ns_rope_seek.c
SRCS += algorithms/ns_rope_write.c
SRCS += algorithms/ns_var_create.c
SRCS += algorithms/ns_var_delete.c
SRCS += algorithms/ns_var_get.c
SRCS += algorithms/ns_var_get_or_create.c
SRCS += algorithms/ns_var_update.c
SRCS += algorithms/ns_write_var_page.c

# Parsers
SRCS += compiler/parsers/ns_parse_multi_user_stride.c
SRCS += compiler/parsers/ns_parse_query.c
SRCS += compiler/parsers/ns_parse_subtype.c
SRCS += compiler/parsers/ns_parse_type.c
SRCS += compiler/parsers/ns_parse_type_ref.c
SRCS += compiler/parsers/ns_parse_user_stride.c

# Compiler
SRCS += compiler/ns_lexer.c
SRCS += compiler/ns_tokens.c

# Page
SRCS += page/ns_page.c
SRCS += page/ns_page_data_list.c
SRCS += page/ns_page_fsm.c
SRCS += page/ns_page_inner_node.c
SRCS += page/ns_page_var_hash_page.c
SRCS += page/ns_page_var_page.c
SRCS += page/ns_page_var_tail.c

# Pager
SRCS += pager/ns_file_pager.c
SRCS += pager/ns_pager.c

# Types
SRCS += types/ns_kvt.c
SRCS += types/ns_query.c
SRCS += types/ns_sarray_t.c
SRCS += types/ns_struct_t.c
SRCS += types/ns_subtype.c
SRCS += types/ns_type_accessor.c
SRCS += types/ns_type_ref.c
SRCS += types/ns_types.c
SRCS += types/ns_union_t.c

# WAL
SRCS += wal/ns_wal.c
SRCS += wal/ns_wal_istream.c
SRCS += wal/ns_wal_ostream.c
SRCS += wal/ns_wal_record.c
SRCS += ns_dirty_page_table.c
SRCS += ns_lock_table.c
SRCS += ns_mem_vhmap.c
SRCS += ns_node_updates.c
SRCS += ns_nsdb.c
SRCS += ns_page_fixture.c
SRCS += ns_txn_table.c
SRCS += ns_variables.c

OBJS := $(SRCS:.c=.o)

TARGET := $(LIB_DIR)/libnscore.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	echo $(CFLAGS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
