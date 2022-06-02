all:
	gcc server.c -o server -lpthread
	gcc client.c -o client -lpthread

clean:
	rm -f server
	rm -f client
