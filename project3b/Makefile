CC = gcc
CFLAGS = -Wall -Werror -Wextra -O3 -g \
-I ./include -I ../hw2/include -I ../hw3a/include -I ../utils

SRCS = ../hw2/src/lamport_queue.c \
../hw3a/src/tas.c ../hw3a/src/ttas.c ../hw3a/src/alock.c ../hw3a/src/clh.c \
../hw3a/src/lock_utils.c src/common.c

UTILS = ../utils/packetsource.c ../utils/generators.c ../utils/crc32.c \
../utils/fingerprint.c ../utils/stopwatch.c

LIBS = -lpthread -pthread -lm

all: driver driver_output

driver: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o driver src/driver.c $(UTILS) $(LIBS)

driver_output: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o tests/driver_output tests/driver_output.c \
	$(UTILS) $(LIBS)

clean:
	rm -rf $(OBJS) driver tests/driver_output *.dSYM tests/*.dSYM tests/*.txt