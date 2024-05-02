CC = gcc
CFLAGS = -Wall -g
DEPS = RUDP_API.h
TARGETS = RUDP_Receiver RUDP_Sender

.PHONY : all clean

all : $(TARGETS)

RUDP_Receiver : RUDP_Receiver.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^

RUDP_Sender : RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^

%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	rm -f *.o *.txt $(TARGETS)
