# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -Ofast -Wno-unused-parameter -Wno-sign-compare -Wunused-function -std=c++17 
LDFLAGS_MT = -lpthread -mconsole
LDFLAGS_ST = -mconsole

# Debug flags
DEBUG_CFLAGS = -DDEBUG -g -O0

# Source files
SRCDIR = src
SRC = $(wildcard $(SRCDIR)/*.cpp)

# Object files - separate for threaded and non-threaded versions
OBJ_MT = $(SRC:%.cpp=%_mt.o)
OBJ_ST = $(SRC:%.cpp=%_st.o)
OBJ_MT_DEBUG = $(SRC:%.cpp=%_mt_debug.o)
OBJ_ST_DEBUG = $(SRC:%.cpp=%_st_debug.o)

# Executable names
TARGET_MT = bin/c2export_multithr.exe
TARGET_ST = bin/c2export_singlethr.exe
TARGET_MT_DEBUG = bin/c2export_multithr_debug.exe
TARGET_ST_DEBUG = bin/c2export_singlethr_debug.exe

all: $(TARGET_MT) $(TARGET_ST)
debug: $(TARGET_MT_DEBUG) $(TARGET_ST_DEBUG)

# Release versions
$(TARGET_MT): $(OBJ_MT)
	$(CXX) -o $@ $^ $(LDFLAGS_MT)

$(TARGET_ST): $(OBJ_ST)
	$(CXX) -o $@ $^ $(LDFLAGS_ST)

# Debug versions
$(TARGET_MT_DEBUG): $(OBJ_MT_DEBUG)
	$(CXX) -o $@ $^ $(LDFLAGS_MT)

$(TARGET_ST_DEBUG): $(OBJ_ST_DEBUG)
	$(CXX) -o $@ $^ $(LDFLAGS_ST)

# Rules for building release object files
%_mt.o: %.cpp
	$(CXX) $(CXXFLAGS) -DCOMPILE_WITH_THREADS=1 -c $< -o $@

%_st.o: %.cpp
	$(CXX) $(CXXFLAGS) -DCOMPILE_WITH_THREADS=0 -c $< -o $@

# Rules for building debug object files
%_mt_debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_CFLAGS) -DCOMPILE_WITH_THREADS=1 -c $< -o $@

%_st_debug.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_CFLAGS) -DCOMPILE_WITH_THREADS=0 -c $< -o $@

# Clean up
clean:
	del /Q src\*_mt.o src\*_st.o src\*_mt_debug.o src\*_st_debug.o

$(TARGET_MT) $(TARGET_ST) $(TARGET_MT_DEBUG) $(TARGET_ST_DEBUG): | bin

bin:
	if not exist bin mkdir bin