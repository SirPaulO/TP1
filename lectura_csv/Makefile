# NOMBRE DEL EJECUTABLE DEL TP
EXEC =  pruebas
CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99 -g
BIN = $(filter-out $(EXEC).c, $(wildcard *.c))
BINFILES = $(BIN:.c=.o)

all: main

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

test: main
	./$(EXEC)

ship: test clean
	tar -czf entrega.tar.gz Makefile *.c *.h
	
main: $(BINFILES)  $(EXEC).c
	$(CC) $(CFLAGS) $(BINFILES) $(EXEC).c -o $(EXEC)

clean:
	rm -f $(wildcard *.o) $(EXEC)
	rm -f entrega.tar.gz

.PHONY: clean main test ship