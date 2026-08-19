C_SRCS :=
C_SRCS += ns_numstore.c
C_SRCS += testing/ns_swarm_tests.c

C_SAMPLES :=
C_SAMPLES += samples/ns_sample1_basic_crud.c

OBJ_DIR := $(OBJ_DIR)/numstore
C_OBJS  := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))

LIB_TARGET := $(LIB_DIR)/libnumstore.a
BIN_TARGET := $(BIN_DIR)/numstore
INC_TARGET := $(INC_DIR)/numstore.h

C_SAMPLE_BINS := $(addprefix $(BIN_DIR)/,$(basename $(notdir $(C_SAMPLES))))
DEPLIBS := $(LIB_TARGET) $(LIB_DIR)/libnscore.a $(LIB_DIR)/libsmartfiles.a $(LIB_DIR)/libcore.a
LDLIBS  := -lnumstore -lnscore -lsmartfiles -lcore

.PHONY: all clean

all: $(LIB_TARGET) $(BIN_TARGET) $(INC_TARGET) $(C_SAMPLE_BINS)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

$(BIN_TARGET): ns_cli.c $(DEPLIBS) $(INC_TARGET) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ -L$(LIB_DIR) $(LDLIBS)

$(INC_TARGET): numstore.h
	cp $^ $@

$(BIN_DIR):
	mkdir -p $@

$(C_SAMPLE_BINS): $(BIN_DIR)/%: samples/%.c $(DEPLIBS) $(INC_TARGET) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ -L$(LIB_DIR) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(LIB_TARGET)
	rm -f $(BIN_TARGET)
	rm -f $(INC_TARGET)
	rm -f $(C_SAMPLE_BINS)
