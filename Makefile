#CFLAGS = -Wall -g 
all: subscriber server

subscriber:
	gcc -Wall -g client.c -o subscriber

server:
	gcc -Wall -g server.c -o server -lm

clean:
	rm -f server subscriber