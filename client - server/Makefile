.PHONY: build run clean

all: build run clean

build:
	gcc -Wall -o server server.c -lm
	gcc -Wall -o subscriber subscriber.c

run:
	./server 5000

clean:
	rm -rf server
	rm -rf subscriber
