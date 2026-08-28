$(BIN_DIR)/nsserver: src/nsserver/nsserver.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

$(BIN_DIR)/nsclient: src/nsserver/nsclient.c $(TARGET_LIB) | $(BIN_DIR)
	@echo "  CC       $< -> $(patsubst $(CURDIR)/%,%,$@)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ $(TARGET_LIB)

ALL += $(BIN_DIR)/nsserver
ALL += $(BIN_DIR)/nsclient
