all:
	gcc -Wall -c common.c
	gcc -Wall -c app.c
	gcc -Wall client.c common.o -o client
	gcc -Wall server.c common.o app.o -lpthread -o server

clean:
	rm common.o client server app.o

debug: all
	gcc -Wall -c -g app.c
	gcc -Wall -g server.c common.o app.o -lpthread -o server