############ Sources

ALL_SRCS += src/numstore/ns_numstore.c
ALL_SRCS += src/numstore/testing/ns_mem_vhmap.c
#ALL_SRCS += src/numstore/testing/ns_numstore_simulation.c

############ Includes

$(INC_DIR)/numstore.h: src/numstore/numstore.h | $(INC_DIR)
	cp $< $@

ALL_HEADERS += $(INC_DIR)/numstore.h

############ Bins

$(BIN_DIR)/numstore: src/numstore/ns_cli.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL_BINS += $(BIN_DIR)/numstore

############ Samples (bins + copied sources), one name list drives both

NS_SAMPLES := ns_sample1_basic_crud

define NS_SAMPLE_RULES
$(BIN_DIR)/$(1): src/numstore/samples/$(1).c $$(TARGET_LIB) $$(INC_DIR)/numstore.h | $$(BIN_DIR)
	$$(CC) $$(CFLAGS) -I$$(INC_DIR) $$< -o $$@ $$(TARGET_LIB)

$(SMP_DIR)/$(1).c: src/numstore/samples/$(1).c | $$(SMP_DIR)
	cp $$< $$@

ALL_BINS += $(BIN_DIR)/$(1)
ALL_SAMPLES += $(SMP_DIR)/$(1).c
endef

$(foreach s,$(NS_SAMPLES),$(eval $(call NS_SAMPLE_RULES,$(s))))
