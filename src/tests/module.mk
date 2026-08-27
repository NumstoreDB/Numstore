.PHONY: src/tests/unit_tests.c
src/tests/unit_tests.c: src/tests/unit_tests.c.in scripts/gen_tests.py
	python3 scripts/gen_tests.py

$(BIN_DIR)/unit_tests: src/tests/unit_tests.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/tests/unit_tests.c -o $@ $(TARGET_LIB)


ALL_BINS += $(BIN_DIR)/unit_tests
