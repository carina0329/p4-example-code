CC=g++ -g -Wall -std=c++17

all: hoh dynmap ondisk

hoh: hoh.o
	${CC} -o $@ $^ -lpthread

dynmap: dynmap.o
	${CC} -o $@ $^ -lpthread

ondisk: ondisk.o libfs_server.o
	${CC} -o $@ $^ -lpthread -ldl

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<

clean:
	rm -f hoh hoh.o dynmap dynmap.o ondisk ondisk.o
