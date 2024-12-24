# Makefile

# Compiler
CC = gcc

# Compiler Flags
# -Wall: Enable all warnings
# -g: Include debugging information
# -I/usr/include/postgresql: Include PostgreSQL headers
CFLAGS = -Wall -g -I/usr/include/postgresql

# Linker Flags
# -lpq: Link with PostgreSQL library
# -lpthread: Link with pthread library
LDFLAGS = -lpq -lpthread

# Source Files
SRCS = main.c client_handler.c database.c session.c chat_room.c

# Object Files
OBJS = $(SRCS:.c=.o)

# Executable Name
EXEC = server

# Default Target
all: $(EXEC)

# Link Object Files to Create Executable
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Up Generated Files
clean:
	rm -f $(OBJS) $(EXEC)
