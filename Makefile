CC=gcc
CFLAGS=-Werror=vla -Wextra -Wall -Wshadow -Wswitch-default -std=c11 
CFLAG_SAN=$(CFLAGS) -fsanitize=address -g -lpthread

.PHONY: clean

clean:
	rm -f *.o
	rm -f all
	rm -f client

server.o: server.c
	$(CC) -c $(CFLAGS) $^ -o $@

dict.o: dict.c
	$(CC) -c $(CFLAGS) $^ -o $@

utilize.o: utilize.c
	$(CC) -c $(CFLAGS) $^ -o $@

queue.o: queue.c
	$(CC) -c $(CFLAGS) $^ -o $@

main.o: main.c
	$(CC) -c $(CFLAGS) $^ -o $@

all: server.o dict.o utilize.o queue.o main.o
	$(CC) $(CFLAG_SAN) $^ -o $@
	./all config_file.bin

client: client.c
	$(CC) -Werror=vla -Wextra -Wall -Wshadow -Wswitch-default -std=c11 -g -fsanitize=address $^ -o $@
	./client
