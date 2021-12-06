all: cli ser

cli: client.c
	gcc -g client.c -o client

ser: server.c
	gcc server.c -g -lpthread -o server
	./server

clean:
	rm server
	rm client
