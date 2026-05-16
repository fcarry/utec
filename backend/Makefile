CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -D_GNU_SOURCE -Isrc
LDFLAGS = -lmicrohttpd -lsqlite3 -lcjson -lpthread

SRCS    = src/main.c src/router.c src/handlers.c src/db.c src/student.c src/http_util.c
OBJS    = $(SRCS:.c=.o)
TARGET  = server

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) students.db
