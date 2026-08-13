C_SRCS := 

C_SRCS += ns_smartfiles.c
C_SRCS += testing/ns_aries_tests.c
C_SRCS += testing/ns_smfile_test_fixture.c

C_OBJS := $(C_SRCS:.c=.o)

LIB_TARGET := $(LIB_DIR)/libsmartfiles.a

.PHONY: all clean
all: $(LIB_TARGET)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJS) $(LIB_TARGET)
