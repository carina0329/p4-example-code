CC=g++ -g -Wall -std=c++17

all: hoh dynmap

hoh: hoh.o
	${CC} -o $@ $^ -lpthread

dynmap: dynmap.o
	${CC} -o $@ $^ -lpthread

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<

clean:
	rm -f *.o hoh dynmap
