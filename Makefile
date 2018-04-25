CC=gcc
CFLAGS=-Wall -g -D_GNU_SOURCE
LDFLAGS=-lreadline

shell: shell.c
	$(CC) $(CFLAGS) shell.c -o shell $(LDFLAGS)

clean:
	rm -rf ./shell
