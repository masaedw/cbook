CFLAGS=-std=c11 -g -static
SRCS=$(filter-out tmp.c, $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(filter-out test/test.c, $(wildcard test/*.c))
TEST_OBJS=$(TEST_SRCS:.c=.o)

hypcc: $(OBJS)
	$(CC) -o hypcc $(OBJS) $(LDFLAGS)

$(OBJS): hypcc.h

test/test: hypcc $(TEST_OBJS) test/test.c
	./hypcc test/test.c > test/test.s
	$(CC) -o test/test $(TEST_OBJS) test/test.s $(LDFLAGS)

test: test/test
	./test/test

clean:
	rm -f hypcc *.o *~ tmp* test/*.o test/*.s test/test

.PHONY: test clean
