#!/usr/bin/env python3
import os
import random
import argparse
import subprocess

def run_cmd(args):
  result = subprocess.run(args, stdout=subprocess.PIPE)
  return result.stdout.decode('utf-8')

def gen_inputs(N):
  dist = [[random.randint(1, 1000) for i in range(N)] for j in range(N)]
  # Choose a random number m from [0, N^2] and assign this many edges to inf
  m = random.randint(0, N * N)
  idx = [(random.randint(0, N - 1), random.randint(0, N - 1)) for _ in range(m)]
  for i, j in idx:
    dist[i][j] = 10000000
  # dist[i][i] should be 0 for all i
  for i in range(N):
    dist[i][i] = 0

  dname = './tests'
  if not os.path.exists(dname):
    os.mkdir(dname)
  fname = os.path.join(dname, 'test{}.txt'.format(N))
  with open(fname, 'w') as fout:
    fout.write('{}\n'.format(N))
    for i in range(N):
      for j in range(N):
        fout.write('{:<10}'.format(dist[i][j]))
      fout.write('\n')
  print('Generated: {}'.format(fname))


def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument('-i', '--generate-inputs', action='store_true')
  parser.add_argument('-r', '--run', action='store_true')
  parser.add_argument('-o', '--check-outputs', action='store_true')
  return parser.parse_args()

def main():
  args = parse_args()

  Ns = [16, 32, 64, 128, 256, 512, 1024]
  Ts = [1, 2, 4, 8, 16, 32, 64]

  if args.generate_inputs:
    print('### Generating Inputs')
    for N in Ns:
      gen_inputs(N)

  if args.run:
    print('{:<10}'.format('N'), end='')
    for N in Ns:
      print('{:<10}'.format(N), end='')

    print('\n{:<10}'.format('Serial'), end='')
    for N in Ns:
      ret = run_cmd(['./shortest_paths', '-f', 'tests/test{}.txt'.format(N)])
      print('{:<10}'.format(ret), end='')

    for T in Ts:
      print('\nT = {:<6}'.format(T), end='')
      for N in Ns:
        ret = run_cmd(['./shortest_paths', '-f', 'tests/test{}.txt'.format(N), 
          '-t', str(T)])
        print('{:<10}'.format(ret), end='')
    print('\n')

  if args.check_outputs:
    print('### Comparing Outputs')
    souts = sorted([f for f in os.listdir('./tests/') if '_s' in f])
    pouts = sorted([f for f in os.listdir('./tests/') if '_p' in f])
    for sout, pout in zip(souts, pouts):
      print('**Testing {} and {}: '.format(sout, pout), end='')
      print(run_cmd(['diff', 'tests/' + sout, 'tests/' + pout]) == '')


if __name__ == '__main__':
  main()