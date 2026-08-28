############ Sources
LIBNS_SRCS += src/smartfiles/ns_smartfiles.c
LIBNS_SRCS += src/smartfiles/testing/ns_aries_tests.c
LIBNS_SRCS += src/smartfiles/testing/ns_smfile_test_fixture.c
LIBNS_SRCS += src/smartfiles/testing/ns_smfile_simulation.c

############ Includes

$(INC_DIR)/smartfiles.h: src/smartfiles/smartfiles.h | $(INC_DIR)
	@echo "  CP       $(patsubst $(CURDIR)/%,%,$<) -> $(patsubst $(CURDIR)/%,%,$@)"
	@cp $< $@

ALL += $(INC_DIR)/smartfiles.h

############ Samples (bins + copied sources), one name list drives both

SMFILE_SAMPLES := 
SMFILE_SAMPLES += smfile_sample1_basic_crud
SMFILE_SAMPLES += smfile_sample2_transactions
SMFILE_SAMPLES += smfile_sample3_stride
SMFILE_SAMPLES += smfile_sample4_rollback_commit

define SMFILE_SAMPLE_RULES
$(BIN_DIR)/$(1): src/smartfiles/samples/$(1).c $$(TARGET_LIB) $$(INC_DIR)/smartfiles.h | $$(BIN_DIR)
	@echo "  CC       $$< -> $$(patsubst $$(CURDIR)/%,%,$$@)"
	@$$(CC) $$(CFLAGS) -I$$(INC_DIR) $$< -o $$@ $$(TARGET_LIB)

$(SMP_DIR)/$(1).c: src/smartfiles/samples/$(1).c | $$(SMP_DIR)
	@echo "  CP       $$(patsubst $$(CURDIR)/%,%,$$<) -> $$(patsubst $$(CURDIR)/%,%,$$@)"
	@cp $$< $$@

ALL += $(BIN_DIR)/$(1)
ALL += $(SMP_DIR)/$(1).c
endef

$(foreach s,$(SMFILE_SAMPLES),$(eval $(call SMFILE_SAMPLE_RULES,$(s))))
