
all: $(BIN_DIR)/nsserver $(BIN_DIR)/nsclient

$(BIN_DIR)/nsserver: $(LIB_DIR)/libcore.a $(LIB_DIR)/libnscore.a nsserver.c
	$(CC) $(CFLAGS) nsserver.c -o $(BIN_DIR)/nsserver -L$(LIB_DIR) -lcore

$(BIN_DIR)/nsclient: $(LIB_DIR)/libcore.a $(LIB_DIR)/libnscore.a nsclient.c
	$(CC) $(CFLAGS) nsclient.c -o $(BIN_DIR)/nsclient -L$(LIB_DIR) -lcore

