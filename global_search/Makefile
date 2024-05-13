CXX = g++
CXXFLAGS = -Wall -O3
LDFLAGS =

# Name of the executable
TARGET = CVRP_Solver

# Source files
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
