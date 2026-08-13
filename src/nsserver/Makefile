SERVER_TARGET := $(BIN_DIR)/nsserver 
CLIENT_TARGET := $(BIN_DIR)/nsclient

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(LIB_DIR)/libcore.a nsserver.c
	$(CC) $(CFLAGS) nsserver.c -o $@ -L$(LIB_DIR) -lcore

$(BIN_DIR)/nsclient: $(LIB_DIR)/libcore.a nsclient.c
	$(CC) $(CFLAGS) nsclient.c -o $@ -L$(LIB_DIR) -lcore

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
