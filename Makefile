CC = g++
LD_FLAG = -levent_core -levent_pthreads -L ~/usr/local/lib
CFLAGS = -g -I. -I ~/usr/local/include

all: server client

server: server.o
	g++ $(LD_FLAG) server.o -o server

client: client.o
	g++ $(LD_FLAG) client.o -o client

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm *.o

.PHONY: clean

