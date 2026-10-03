############ Sources

LIBNS_SRCS += src/numstore/numstore.c

############ Includes

$(INC_DIR)/numstore.h: src/numstore.h | $(INC_DIR)
	cp $< $@

############ Bins

$(BIN_DIR)/numstore: src/numstore/ns_cli.c $(TARGET_LIB) | $(BIN_DIR)
	$(LINK_BIN)

ALL += $(INC_DIR)/numstore.h
ALL += $(BIN_DIR)/numstore

############ Samples (bins + copied sources), one name list drives both

NS_SAMPLES := ns_sample1_basic_crud
NS_SAMPLES += ns_big_file

define NS_SAMPLE_RULES

$(BIN_DIR)/$(1): src/numstore/samples/$(1).c $$(TARGET_LIB) $$(INC_DIR)/numstore.h | $$(BIN_DIR)
	$$(LINK_BIN)

$(SMP_DIR)/$(1).c: src/numstore/samples/$(1).c | $$(SMP_DIR)
	cp $$< $$@

ALL += $(BIN_DIR)/$(1)
ALL += $(SMP_DIR)/$(1).c
endef

$(foreach s,$(NS_SAMPLES),$(eval $(call NS_SAMPLE_RULES,$(s))))
