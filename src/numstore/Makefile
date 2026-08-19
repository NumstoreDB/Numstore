C_SRCS :=

C_SRCS += ns_cli.c
C_SRCS += ns_numstore.c

C_OBJS := $(C_SRCS:.c=.o)

UNIT_TESTS_SRC := $(CURDIR)/../unit_tests.c
UNIT_TESTS_OBJ := $(CURDIR)/../unit_tests.o

LIB_TARGET := $(LIB_DIR)/libnumstore.a
BIN_TARGET := $(BIN_DIR)/numstore

.PHONY: all clean
all: $(LIB_TARGET) $(BIN_TARGET)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

$(UNIT_TESTS_OBJ): $(UNIT_TESTS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_TARGET): $(C_OBJS) $(UNIT_TESTS_OBJ) $(LIB_DIR)/libnscore.a $(LIB_DIR)/libsmartfiles.a $(LIB_DIR)/libcore.a
	$(CC) $(CFLAGS) $(C_OBJS) $(UNIT_TESTS_OBJ) -o $@ -L$(LIB_DIR) -lnscore -lsmartfiles -lcore

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJS) $(UNIT_TESTS_OBJ) $(LIB_TARGET) $(BIN_TARGET)
