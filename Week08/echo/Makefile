CC = gcc
CFLAGS = -Wall -O2
OBJS = csapp.o
TARGETS = echoserveri echoclient

all: $(TARGETS)

echoserveri: echoserveri.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ echoserveri.o $(OBJS)

echoclient: echoclient.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ echoclient.o $(OBJS)

clean:
	rm -f *.o $(TARGETS)