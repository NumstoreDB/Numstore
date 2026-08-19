C_SRCS :=
C_SRCS += ns_alloc.c
C_SRCS += ns_block_array.c
C_SRCS += ns_byte_accessor.c
C_SRCS += ns_cbuffer.c
C_SRCS += ns_chunk_alloc.c
C_SRCS += ns_concurrency.c
C_SRCS += ns_data_validator.c
C_SRCS += ns_dbl_buffer.c
C_SRCS += ns_error.c
C_SRCS += ns_ext_array.c
C_SRCS += ns_htable.c
C_SRCS += ns_linked_list.c
C_SRCS += ns_logging.c
C_SRCS += ns_numerics.c
C_SRCS += ns_robin_hood_ht.c
C_SRCS += ns_serial.c
C_SRCS += ns_slab_alloc.c
C_SRCS += ns_stream.c
C_SRCS += ns_stride.c
C_SRCS += ns_string.c
C_SRCS += ns_utils.c
C_SRCS += os/common/ns_memory.c
C_SRCS += os/posix/ns_posix_file.c
C_SRCS += os/posix/ns_posix_filesystem.c
C_SRCS += os/posix/ns_posix_threading.c
C_SRCS += os/posix/ns_posix_timing.c
C_SRCS += os/windows/ns_win32_filesystem.c
C_SRCS += os/windows/ns_win32_threading.c
C_SRCS += os/windows/ns_win32_timing.c
C_SRCS += testing/ns_testing.c
C_SRCS += testing/ns_tests.c

C_OBJS := $(C_SRCS:.c=.o)

LIB_TARGET := $(LIB_DIR)/libcore.a

.PHONY: all clean
all: $(LIB_TARGET)

$(LIB_TARGET): $(C_OBJS) 
	$(AR) rcs $@ $(C_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJS) $(LIB_TARGET) 
