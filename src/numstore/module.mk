############ Sources

LIBNS_SRCS += src/numstore/ns_numstore.c
LIBNS_SRCS += src/numstore/testing/ns_mem_vhmap.c
LIBNS_SRCS += src/numstore/testing/ns_actual_db_stepper.c
LIBNS_SRCS += src/numstore/testing/ns_reference_db_stepper.c
LIBNS_SRCS += src/numstore/testing/ns_operation_generator.c
LIBNS_SRCS += src/numstore/testing/ns_numstore_simulation.c

############ Includes

$(INC_DIR)/numstore.h: src/numstore/numstore.h | $(INC_DIR)
	@echo "  CP       $(patsubst $(CURDIR)/%,%,$<) -> $(patsubst $(CURDIR)/%,%,$@)"
	@cp $< $@

############ Bins

$(BIN_DIR)/numstore: src/numstore/ns_cli.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL += $(INC_DIR)/numstore.h
ALL += $(BIN_DIR)/numstore

############ Samples (bins + copied sources), one name list drives both

NS_SAMPLES := ns_sample1_basic_crud
NS_SAMPLES += ns_big_file

define NS_SAMPLE_RULES

$(BIN_DIR)/$(1): src/numstore/samples/$(1).c $$(TARGET_LIB) $$(INC_DIR)/numstore.h | $$(BIN_DIR)
	@echo "  CC       $$< -> $$(patsubst $$(CURDIR)/%,%,$$@)"
	@$$(CC) $$(CFLAGS) -I$$(INC_DIR) $$< -o $$@ $$(TARGET_LIB)

$(SMP_DIR)/$(1).c: src/numstore/samples/$(1).c | $$(SMP_DIR)
	@echo "  CP       $$(patsubst $$(CURDIR)/%,%,$$<) -> $$(patsubst $$(CURDIR)/%,%,$$@)"
	@cp $$< $$@

ALL += $(BIN_DIR)/$(1)
ALL += $(SMP_DIR)/$(1).c
endef

$(foreach s,$(NS_SAMPLES),$(eval $(call NS_SAMPLE_RULES,$(s))))
