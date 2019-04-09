#!/usr/bin/env python3
import sys
import random

def main():
  if len(sys.argv) != 3:
    print("Usage: ./test.py N OUTFILE")
    sys.exit(1)
  N = int(sys.argv[1])
  outfname = sys.argv[2]

  dist = [[random.randint(-1000, 1000) for i in range(N)] for j in range(N)]
  # dist[i][i] should be 0 for all i
  for i in range(N):
    dist[i][i] = 0
  # Choose a random number m from [0, N^2] and assign this many edges to inf
  m = random.randint(0, N * N)
  idx = [(random.randint(0, N - 1), random.randint(0, N - 1)) for _ in range(m)]
  for i, j in idx:
    dist[i][j] = 10000000

  with open(outfname, 'w') as fout:
    for i in range(N):
      for j in range(N):
        fout.write('{:<10}'.format(dist[i][j]))
      fout.write('\n')

if __name__ == '__main__':
  main()