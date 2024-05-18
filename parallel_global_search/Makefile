# Makefile for MPI + OpenMP program

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -O3 -fopenmp -std=c++17

# Target executable name
TARGET = CVRP_Solver

# Source files
SOURCES = main.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Build all targets
all: $(TARGET)

# Rule to link the object files into the executable
# The -o $@ says to put the output of the compilation in the file named on the left side of the :
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Rule to compile the source files into object files
# The -c flag says to generate the object file,
# the -o $@ says to put the output of the compilation in the file named on the left side of the :
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJECTS)

# Declare a "phony" target to handle conflicts if any files are named as a target
.PHONY: all clean
