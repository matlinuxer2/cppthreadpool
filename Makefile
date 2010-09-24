CXX=g++
LDFLAGS=-pthread
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
BIN=threadpool

.PHONY: all clean

all: $(OBJ)
	$(CXX) $? $(LDFLAGS) -o $(BIN)

clean:
	$(RM) $(OBJ) $(BIN)

.cpp.o:
	$(CXX) $< -c $(CXXFLAGS) -o $@
