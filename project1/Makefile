all: shortest_paths

shortest_paths: shortest_paths.c
	gcc shortest_paths.c ../utils/stopwatch.c -o shortest_paths -Werror -Wextra -Wall -O3 -g -lpthread

clean:
	rm -rf shortest_paths
