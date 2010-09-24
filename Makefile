CXX=g++
LDFLAGS=-pthread
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
DEP=$(SRC:.cpp=.d)
BIN=threadpool

.PHONY: all clean

all: $(OBJ)
	$(CXX) $? $(LDFLAGS) -o $(BIN)

clean:
	$(RM) $(OBJ) $(BIN)

%.o: %.cpp %.d
	$(CXX) $< -c $(CXXFLAGS) -o $@

%.d: %.cpp
	$(CXX) -MM $< -o $@

-include $(DEP)
