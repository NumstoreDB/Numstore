C_SRCS := 

C_SRCS += ns_cli.c
C_SRCS += ns_numstore.c

C_OBJS := $(C_SRCS:.c=.o)

LIB_TARGET := $(LIB_DIR)/libnumstore.a

.PHONY: all clean
all: $(LIB_TARGET)

$(LIB_TARGET): $(C_OBJS)
	$(AR) rcs $@ $(C_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJS) $(LIB_TARGET)
