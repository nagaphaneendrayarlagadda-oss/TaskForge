CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -pthread

SERVER = taskforge
CLIENT = taskctl

SERVER_SRC = src/main.c \
             src/scheduler.c \
             src/thread_pool.c \
             src/monitor.c \
             src/ipc.c

CLIENT_SRC = src/taskctl.c \
             src/ipc.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $(SERVER) $(LDFLAGS)

$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $(CLIENT) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER) $(CLIENT)

run: $(SERVER)
	./$(SERVER)

.PHONY: all clean run
