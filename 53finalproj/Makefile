CC=gcc
CFLAGS=-Iinclude -Wall -Werror -g -Wno-unused

CHSRC=$(shell find src/chat -name '*.c')
SSRC=$(shell find src/server -name '*.c')
DEPS=$(shell find include -name '*.h')

LIBS=-lpthread

all: setup server chat

setup:
	mkdir -p bin 
	cp lib/petr_client bin/petr_client

server: setup
	$(CC) $(CFLAGS) $(SSRC) lib/protocol.o -o bin/petr_server $(LIBS)

chat: setup $(DEPS)
	$(CC) $(CFLAGS) $(CHSRC) lib/chat.o -o bin/petr_chat
	
.PHONY: clean

clean:
	rm -rf bin 
