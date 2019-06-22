#!/usr/bin/env python3
import itertools
import subprocess

def run_cmd(args):
  result = subprocess.run(args, stdout=subprocess.PIPE)
  return result.stdout.decode('utf-8')

def main():
  n = 14
  T = 4000
  D = 32
  W = 200
  # ./driver_output -n 3 -t 5 -d 32 -w 25 -p [c|u|e] -r [s|p|q]
  print('### Running Programs')
  for prog in ['s', 'p', 'q']:
    run_cmd(['./driver_output', '-n', str(n), '-t', str(T), '-d', str(D),
      '-w', str(W), '-p', 'e', '-r', prog])

  print('### Comparing Outputs')
  files = ['s_res.txt', 'p_res.txt', 'q_res.txt']
  for a, b in list(itertools.combinations(files, 2)):
    print('**Testing {} and {}: '.format(a, b), end='')
    print(run_cmd(['diff', a, b]) == '')

if __name__ == '__main__':
  main()