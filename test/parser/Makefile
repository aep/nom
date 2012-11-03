CXXFLAGS := -std=c++11

SRC=main.cpp ../../make/lexer.cpp ../../make/parser.cpp
TARGET=parser

OBJ=$(SRC:%.cpp=%.o)

$(TARGET): $(OBJ)
	g++ $^ -o $@

clean:
	rm -f *.o
	rm -f $(OBJ)
	rm -f $(TARGET)
