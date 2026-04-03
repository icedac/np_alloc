CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread
INCLUDES = -I. -I../src -include linux_stdafx.h

SRCDIR = ../src
TESTDIR = ../np_alloc_test

# Source files
SRCS = \
    $(SRCDIR)/np_alloc.cpp \
    $(SRCDIR)/np_mmap.cpp \
    np_thread_linux.cpp \
    $(TESTDIR)/test.cpp \
    linux_main.cpp

# We need to handle stdafx.h — force-include our linux version
# and skip the Windows stdafx.cpp

OBJS = np_alloc.o np_mmap.o np_thread_linux.o test.o linux_main.o

TARGET = np_alloc_bench

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

np_alloc.o: $(SRCDIR)/np_alloc.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

np_mmap.o: $(SRCDIR)/np_mmap.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

np_thread_linux.o: np_thread_linux.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test.o: $(TESTDIR)/test.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -O2 -Wall -pthread -I. -I../src -include linux_stdafx.h -c $< -o $@

linux_main.o: linux_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
