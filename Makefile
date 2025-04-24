CC = g++

CFLGS = -std=c++17
CLIBS = -pthread
GLLIBS = -lglfw -lGLEW -lGL -lm

all: obj.o main.cpp
	$(CC) $(CFLGS) -o main main.cpp $(CLIBS) $(GLLIBS) obj.o

obj.o: obj.cpp obj.hpp rapidobj.hpp
	$(CC) $(CFLGS) -c obj.cpp

clean:
	rm -f main *.o
