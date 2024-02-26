# Use the gcc compiler.
CC = gcc

# Flags for the compiler.
CFLAGS = -Wall -Wextra -g

.PHONY: all default clean

all: TCP_Sender TCP_Receiver

TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) -o $@ $^


TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) -o $@ $^


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o TCP_Receiver TCP_Sender
