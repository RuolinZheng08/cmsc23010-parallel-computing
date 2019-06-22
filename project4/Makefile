CC = gcc
CFLAGS = -Wall -Werror -Wextra -O3 -g \
-I ./include -I ../hw2/include -I ../utils

SRCS = ../hw2/src/lamport_queue.c src/ht_utils.c src/ht_finelock.c

UTILS = ../utils/hashtable.c ../utils/seriallist.c ../utils/hashgenerator.c \
../utils/packetsource.c ../utils/generators.c ../utils/crc32.c \
../utils/fingerprint.c ../utils/stopwatch.c

LIBS = -lpthread -pthread -lm

all: driver tests

driver: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o driver src/driver.c $(UTILS) $(LIBS)

tests: test_hashtable_one_thread test_hashtable_concurrent

test_hashtable_one_thread:
	$(CC) $(CFLAGS) $(SRCS) -o tests/test_hashtable_one_thread \
	tests/test_hashtable_one_thread.c $(UTILS) -lcriterion $(LIBS)

test_hashtable_concurrent:
	$(CC) $(CFLAGS) $(SRCS) -o tests/test_hashtable_concurrent \
	tests/test_hashtable_concurrent.c $(UTILS) $(LIBS)

clean:
	rm -rf $(OBJS) driver tests/test_hashtable_one_thread \
	tests/test_hashtable_concurrent *.dSYM tests/*.dSYM tests/*.txt