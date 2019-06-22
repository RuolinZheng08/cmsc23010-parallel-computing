CC = gcc
CFLAGS = -Wall -Werror -Wextra -O3 -g -I ./include -I ../utils
OBJS = src/driver.o src/lamport_queue.o
SRCS = src/driver.c src/lamport_queue.c
UTILS = ../utils/packetsource.c ../utils/generators.c ../utils/crc32.c \
../utils/fingerprint.c ../utils/stopwatch.c
LIBS = -lpthread -pthread -lm

all: driver tests

driver: $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o driver $(UTILS) $(LIBS)

$(OBJS): $(SRCS) $(UTILS)
	$(CC) $(CFLAGS) -c -o $@ $<

tests: driver_output test_queue

driver_output:
	$(CC) $(CFLAGS) -o tests/driver_output tests/driver_output.c src/lamport_queue.c \
	$(UTILS) $(LIBS)

test_queue:
	$(CC) $(CFLAGS) -o tests/test_queue tests/test_queue.c src/lamport_queue.c \
	$(UTILS) -lcriterion $(LIBS)

clean:
	rm -rf $(OBJS) driver tests/driver_output tests/test_queue \
	*.dSYM tests/*.dSYM tests/*.txt
