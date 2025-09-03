# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -Ofast -Wno-unused-parameter -Wno-sign-compare -Wunused-function -std=c++17
LDFLAGS = -mconsole
DEBUG_CFLAGS = -DDEBUG -g -O0

SRCDIR = src

rwildcard = $(strip \
  $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)) \
  $(wildcard $1$2))

SRC := $(call rwildcard,$(SRCDIR)/,*.cpp)

# Object files
OBJ        := $(patsubst %.cpp,%.o,$(SRC))
OBJ_DEBUG  := $(patsubst %.cpp,%_debug.o,$(SRC))

# Executables
TARGET        = bin/c2export_gcc.exe
TARGET_DEBUG  = bin/c2export_gcc_debug.exe

.PHONY: all debug clean
all:   $(TARGET)
debug: $(TARGET_DEBUG)

# Link
$(TARGET): $(OBJ) | bin
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TARGET_DEBUG): $(OBJ_DEBUG) | bin
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%_debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

CLEAN_OBJS := $(call rwildcard,$(SRCDIR)/,*.o)
clean:
	del /Q src\*.o
	del /Q src\build\*.o
	del /Q src\utils\*.o
	del /Q src\side_scripts\*.o

$(TARGET) $(TARGET_DEBUG) : | bin

bin:
	if not exist bin mkdir bin
