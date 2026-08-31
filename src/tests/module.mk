.PHONY: src/tests/unit_tests.c
src/tests/unit_tests.c: src/tests/unit_tests.c.in scripts/gen_tests.py
	python3 scripts/gen_tests.py

$(BIN_DIR)/unit_tests: src/tests/unit_tests.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/numstore_simulation_test: src/tests/numstore_simulation_test.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL += $(BIN_DIR)/unit_tests
ALL += $(BIN_DIR)/numstore_simulation_test
