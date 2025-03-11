# Compiler and flags
CC = clang
CFLAGS = -Wall -Wextra -g

# Source and object files
SRCS = cards.c
OBJS = cards.o

# Default target
all: cards

# Compile cards.c into an object file
cards: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o cards

# Compile object file, with dependencies on headers
cards.o: cards.c cards.h klondike.h
	$(CC) $(CFLAGS) -c cards.c -o cards.o

# Clean up
clean:
	rm -f cards cards.o

