############ Sources
ALL_SRCS += src/smartfiles/ns_smartfiles.c
ALL_SRCS += src/smartfiles/testing/ns_aries_tests.c
ALL_SRCS += src/smartfiles/testing/ns_smfile_test_fixture.c
ALL_SRCS += src/smartfiles/testing/ns_smfile_simulation.c

############ Includes
$(INC_DIR)/smartfiles.h: src/smartfiles/smartfiles.h | $(INC_DIR)
	cp $< $@

ALL_HDRS += $(INC_DIR)/smartfiles.h

############ Bins
$(BIN_DIR)/smfile_sample1_basic_crud: src/smartfiles/samples/smfile_sample1_basic_crud.c $(TARGET_LIB) $(INC_DIR)/smartfiles.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/smfile_sample2_transactions: src/smartfiles/samples/smfile_sample2_transactions.c $(TARGET_LIB) $(INC_DIR)/smartfiles.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/smfile_sample3_stride: src/smartfiles/samples/smfile_sample3_stride.c $(TARGET_LIB) $(INC_DIR)/smartfiles.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/smfile_sample4_rollback_commit: src/smartfiles/samples/smfile_sample4_rollback_commit.c $(TARGET_LIB) $(INC_DIR)/smartfiles.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL_BINS += $(BIN_DIR)/smfile_sample1_basic_crud
ALL_BINS += $(BIN_DIR)/smfile_sample2_transactions
ALL_BINS += $(BIN_DIR)/smfile_sample3_stride
ALL_BINS += $(BIN_DIR)/smfile_sample4_rollback_commit
