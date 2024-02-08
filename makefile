CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Werror -Wno-error=unused-but-set-variable -Wno-implicit-function-declaration

.PHONY: all clean

all: worker oss

worker:	worker.c
	$(CC) $(CFLAGS) -o worker worker.c

oss: oss.c
	$(CC) $(CFLAGS) -o oss oss.c

clean:
	rm -f worker oss
