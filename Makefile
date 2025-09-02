# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -Ofast -Wno-unused-parameter -Wno-sign-compare -Wunused-function -std=c++17 
LDFLAGS = -mconsole

# Debug flags
DEBUG_CFLAGS = -DDEBUG -g -O0

# Source files
SRCDIR = src
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRC := $(call rwildcard,$(SRCDIR)/,*.cpp)

# Object files - separate for threaded and non-threaded versions
OBJ = $(SRC:%.cpp=%.o)
OBJ_DEBUG = $(SRC:%.cpp=%_debug.o)

# Executable names
TARGET_MT = bin/c2export_gcc.exe
TARGET_MT_DEBUG = bin/c2exportgcc_debug.exe

all: $(TARGET_MT) 
debug: $(TARGET_MT_DEBUG) 

# Release versions
$(TARGET_MT): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Debug versions
$(TARGET_MT_DEBUG): $(OBJ_DEBUG)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Rules for building release object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rules for building debug object files
%_debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

# Clean up
clean:
	del /Q src\*.o

$(TARGET_MT) $(TARGET_MT_DEBUG) : | bin

bin:
	if not exist bin mkdir bin