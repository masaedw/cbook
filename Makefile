CFLAGS=-std=c11 -g -static
SRCS=$(filter-out tmp.c, $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

hypcc: $(OBJS)
	$(CC) -o hypcc $(OBJS) $(LDFLAGS)

$(OBJS): hypcc.h

test: hypcc
	./test.sh

clean:
	rm -f hypcc *.o *~ tmp*

.PHONY: test clean
