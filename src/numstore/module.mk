############ Sources
ALL_SRCS += src/numstore/ns_numstore.c
#ALL_SRCS += src/numstore/testing/ns_numstore_simulation.c

############ Includes

$(INC_DIR)/numstore.h: src/numstore/numstore.h | $(INC_DIR)
	cp $< $@

ALL_HDRS += $(INC_DIR)/smartfiles.h

############ Bins

$(BIN_DIR)/numstore: src/numstore/ns_cli.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/numstore/ns_cli.c -o $@ $(TARGET_LIB)

$(BIN_DIR)/ns_sample1_basic_crud: src/numstore/samples/ns_sample1_basic_crud.c $(TARGET_LIB) $(INC_DIR)/numstore.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/numstore/samples/ns_sample1_basic_crud.c -o $@ $(TARGET_LIB)

ALL_BINS += $(BIN_DIR)/ns_sample1_basic_crud
ALL_BINS += $(BIN_DIR)/numstore

