#!/usr/bin/env python3
import subprocess
from statistics import mean, stdev

def run_cmd(args):
  result = subprocess.run(args, stdout=subprocess.PIPE)
  return result.stdout.decode('utf-8')

def main():
  big = 4000000
  n = 4
  locks = ['tas', 'ttas', 'mutex', 'clh', 'alock']

  # ./driver_output -b 100000 -n 4 -l [tas|ttas|alock|clh|mutex]
  print('### Running Programs\n')
  print('**Running serial')
  run_cmd(['./driver_output', '-b', str(big)])
  for l in locks:
    print('**Running {}'.format(l))
    run_cmd(['./driver_output', '-b', str(big), '-n', str(n), '-l', l])

  print('\n### Comparing Outputs\n')
  files = [l + '_res.txt' for l in locks]
  for f in files:
    print('**Testing serial_res.txt and {}: '.format(f), end='')
    print(run_cmd(['diff', 'serial_res.txt', f]) == '')

  print('\n### Testing Fairness\n')
  files = [l + '_fifo.txt' for l in locks]
  for f in files:
    print('**Testing {}'.format(f))
    with open(f, 'r') as fin:
      counts = [int(count) for count in fin.read().split('\n') if count != '']
    print('  max: {}, min: {}, mean: {}, standard deviation: {}'.format(
      max(counts), min(counts), mean(counts), stdev(counts)))

if __name__ == '__main__':
  main()
