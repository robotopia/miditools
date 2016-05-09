CC=gcc
CFLAGS=-Wall -O2 -Wno-unused-result
TARGETS=swing split

all: $(TARGETS)

swing: common.o swing.o

split: common.o split.o

clean:
	-rm -f *.o $(TARGETS)
