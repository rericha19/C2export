# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Ofast -Wno-unused-parameter -Wno-sign-compare -Wunused-function
LDFLAGS = -lpthread

# Debug flags
DEBUG_CFLAGS = -DDEBUG -g -O0

# Source files
SRCDIR = src
SRC = $(wildcard $(SRCDIR)/*.c)

# Object files - separate for threaded and non-threaded versions
OBJ_MT = $(SRC:%.c=%_mt.o)
OBJ_ST = $(SRC:%.c=%_st.o)
OBJ_MT_DEBUG = $(SRC:%.c=%_mt_debug.o)
OBJ_ST_DEBUG = $(SRC:%.c=%_st_debug.o)

# Executable names
TARGET_MT = bin/c2export_multithr.exe
TARGET_ST = bin/c2export_singlethr.exe
TARGET_MT_DEBUG = bin/c2export_multithr_debug.exe
TARGET_ST_DEBUG = bin/c2export_singlethr_debug.exe

all: $(TARGET_MT) $(TARGET_ST)
debug: $(TARGET_MT_DEBUG) $(TARGET_ST_DEBUG)

# Release versions
$(TARGET_MT): $(OBJ_MT)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET_ST): $(OBJ_ST)
	$(CC) -o $@ $^

# Debug versions
$(TARGET_MT_DEBUG): $(OBJ_MT_DEBUG)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET_ST_DEBUG): $(OBJ_ST_DEBUG)
	$(CC) -o $@ $^

# Rules for building release object files
%_mt.o: %.c
	$(CC) $(CFLAGS) -DCOMPILE_WITH_THREADS=1 -c $< -o $@

%_st.o: %.c
	$(CC) $(CFLAGS) -DCOMPILE_WITH_THREADS=0 -c $< -o $@

# Rules for building debug object files
%_mt_debug.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -DCOMPILE_WITH_THREADS=1 -c $< -o $@

%_st_debug.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -DCOMPILE_WITH_THREADS=0 -c $< -o $@

# Clean up
clean:
	del /Q src\*_mt.o src\*_st.o src\*_mt_debug.o src\*_st_debug.o	

$(TARGET_MT) $(TARGET_ST) $(TARGET_MT_DEBUG) $(TARGET_ST_DEBUG): | bin

bin:
	if not exist bin mkdir bin