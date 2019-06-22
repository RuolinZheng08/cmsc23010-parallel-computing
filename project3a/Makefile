CC = gcc
CFLAGS = -Wall -Werror -Wextra -O3 -g -I ./include -I ../utils
SRCS = src/tas.c src/ttas.c src/alock.c src/clh.c src/lock_utils.c
OBJS = src/tas.o src/ttas.o src/alock.o src/clh.o src/lock_utils.o
UTILS = ../utils/stopwatch.c
LIBS = -lpthread -pthread

all: driver tests

tests: driver_output test_locks

driver: $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o driver src/driver.c $(UTILS) $(LIBS)

$(OBJS): $(SRCS) $(UTILS)
	$(CC) $(CFLAGS) -c -o $@ $<

test_locks:
	$(CC) $(CFLAGS) $(SRCS) -o tests/test_locks tests/test_alock.c \
	tests/test_clh.c -lcriterion $(UTILS) $(LIBS)

driver_output: $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o tests/driver_output tests/driver_output.c \
	$(UTILS) $(LIBS)

clean:
	rm -rf $(OBJS) driver tests/driver_output tests/test_locks \
	*.dSYM tests/*.dSYM tests/*.txt