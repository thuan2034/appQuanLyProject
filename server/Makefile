CC=gcc
CFLAGS=-Wall -g -I/usr/include/postgresql
LDFLAGS=-lpq -lpthread

SRCS=main.c client_handler.c database.c session.c
OBJS=$(SRCS:.c=.o)
EXEC=server

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXEC)
