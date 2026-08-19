C_SRCS :=
C_SRCS += ns_smartfiles.c
C_SRCS += testing/ns_aries_tests.c
C_SRCS += testing/ns_smfile_test_fixture.c

C_SAMPLES :=
C_SAMPLES += samples/smfile_sample1_basic_crud.c
C_SAMPLES += samples/smfile_sample2_transactions.c
C_SAMPLES += samples/smfile_sample3_stride.c
C_SAMPLES += samples/smfile_sample4_rollback_commit.c

OBJ_DIR := $(OBJ_DIR)/smartfiles
C_OBJS  := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))

LIB_TARGET := $(LIB_DIR)/libsmartfiles.a
INC_TARGET := $(INC_DIR)/smartfiles.h

C_SAMPLE_BINS := $(addprefix $(BIN_DIR)/,$(basename $(notdir $(C_SAMPLES))))

# Consumer-before-provider: smartfiles needs nscore, nscore needs core
DEPLIBS := $(LIB_TARGET) $(LIB_DIR)/libnscore.a $(LIB_DIR)/libcore.a
LDLIBS  := -lsmartfiles -lnscore -lcore

.PHONY: all clean
all: $(LIB_TARGET) $(INC_TARGET) $(C_SAMPLE_BINS)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

$(INC_TARGET): smartfiles.h
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
	rm -f $(INC_TARGET)
	rm -f $(C_SAMPLE_BINS)
