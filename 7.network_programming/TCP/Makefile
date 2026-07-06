all: server client

server: server.c
	gcc -Wall -Wextra -O2 -o $@ $<

client: client.c
	gcc -Wall -Wextra -O2 -o $@ $<

clean:
	rm -f server client

.PHONY: all clean