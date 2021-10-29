CFLAGS = -std=gnu11 -I src

all: mytftpclient

mytftpclient: src/args.c src/client.c src/connection.c src/main.c src/util.c
	$(CC) $(CFLAGS) $^ -o $@

pack:
	tar -cvf xkalaz00.tar src/* Makefile README manual.pdf

clean:
	rm -f mytftpclient
