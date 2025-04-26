# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O3 -Wno-unused-parameter -Wno-sign-compare
LDFLAGS = -lpthread

# Source files
SRCDIR = src
SRC = $(wildcard $(SRCDIR)/*.c)

# Object files - separate for threaded and non-threaded versions
OBJ_MT = $(SRC:%.c=%_mt.o)
OBJ_ST = $(SRC:%.c=%_st.o)

# Executable names
TARGET_MT = bin/c2export_multithr.exe
TARGET_ST = bin/c2export_singlethr.exe

all: $(TARGET_MT) $(TARGET_ST)

# Threaded version
$(TARGET_MT): $(OBJ_MT)
	$(CC) -o $@ $^ $(LDFLAGS)

# Non-threaded version
$(TARGET_ST): $(OBJ_ST)
	$(CC) -o $@ $^

# Rule for building threaded object files
%_mt.o: %.c
	$(CC) $(CFLAGS) -DCOMPILE_WITH_THREADS=1 -c $< -o $@

# Rule for building non-threaded object files
%_st.o: %.c
	$(CC) $(CFLAGS) -DCOMPILE_WITH_THREADS=0 -c $< -o $@

# Clean up
clean:
	del /Q src\*_mt.o src\*_st.o

# Create bin directory if it doesn't exist
$(TARGET_MT) $(TARGET_ST): | bin

bin:
	if not exist bin mkdir bin