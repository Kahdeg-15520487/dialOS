# Makefile for dialScript Parser
# Compiles the parser for standalone PC use

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = test_parser
SOURCES = test_parser.cpp lexer.cpp ast.cpp parser.cpp ast_printer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run the test
test: $(TARGET)
	./$(TARGET)

# Dependencies
lexer.o: lexer.cpp lexer.h
ast.o: ast.cpp ast.h
parser.o: parser.cpp parser.h lexer.h ast.h
ast_printer.o: ast_printer.cpp ast_printer.h ast.h
test_parser.o: test_parser.cpp lexer.h parser.h ast_printer.h

.PHONY: all clean test
