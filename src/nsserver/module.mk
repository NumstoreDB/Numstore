$(BIN_DIR)/nsserver: src/nsserver/nsserver.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/nsserver/nsserver.c -o $@ $(TARGET_LIB)

$(BIN_DIR)/nsclient: src/nsserver/nsclient.c $(TARGET_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) src/nsserver/nsclient.c -o $@ $(TARGET_LIB)

ALL_BINS += $(BIN_DIR)/nsserver
ALL_BINS += $(BIN_DIR)/nsclient
