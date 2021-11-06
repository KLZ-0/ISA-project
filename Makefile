CFLAGS = -std=gnu11 -I src

all: mytftpclient
.PHONY: pack clean importdoc

mytftpclient: src/args.c src/client.c src/connection.c src/main.c src/util.c
	$(CC) $(CFLAGS) $^ -o $@

pack:
	tar -cvf xkalaz00.tar src/* Makefile README manual.pdf

importdoc:
	cp ../doc/out/main.pdf manual.pdf

clean:
	rm -f mytftpclient
