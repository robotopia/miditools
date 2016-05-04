CC=gcc
CFLAGS=-Wall -O2
TARGETS=swing

all: $(TARGETS)

swing: common.o swing.o

clean:
	-rm -f *.o $(TARGETS)
