$(BIN_DIR)/nsserver: src/nsserver/nsserver.c $(TARGET_LIB) | $(BIN_DIR)
	$(LINK_BIN)

$(BIN_DIR)/nsclient: src/nsserver/nsclient.c $(TARGET_LIB) | $(BIN_DIR)
	$(LINK_BIN)

ALL += $(BIN_DIR)/nsserver
ALL += $(BIN_DIR)/nsclient
