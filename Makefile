# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O3 -Wno-unused-parameter -Wno-sign-compare
LDFLAGS = -lpthread

# Source files
SRCDIR = src
SRC = $(wildcard $(SRCDIR)/*.c)

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET = bin/c2export_dl.exe

# Build target
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Rule for building object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	del /Q src\*.o
