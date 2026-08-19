SERVER_TARGET := $(BIN_DIR)/nsserver
CLIENT_TARGET := $(BIN_DIR)/nsclient

DEPLIBS := $(LIB_DIR)/libcore.a
LDLIBS  := -lcore

.PHONY: all clean
all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(BIN_DIR):
	mkdir -p $@

$(SERVER_TARGET): nsserver.c $(DEPLIBS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ -L$(LIB_DIR) $(LDLIBS)

$(CLIENT_TARGET): nsclient.c $(DEPLIBS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< -o $@ -L$(LIB_DIR) $(LDLIBS)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
