#!/usr/bin/env python3
import itertools
import subprocess

def run_cmd(args):
  result = subprocess.run(args, stdout=subprocess.PIPE)
  return result.stdout.decode('utf-8')

def main():
  t = 10000
  n = 7
  w = 2000
  u = 1
  d = 8
  locks = ['mutex', 'clh']
  strategies = ['homequeue', 'awesome']
  combo = list(itertools.product(locks, strategies))
  serial_cmd = ['./driver_output', '-t', str(t), '-n', str(n), '-w', str(w), '-u', '1']

  print('### Running Programs')
  print('num_src: {}, num_pkt: {}, mean: {}, uniform, depth: {}\n'.format(n, t, w, d))
  print('**Running serial')
  # ./driver_output -t 1000 -n 7 -w 1000 -u 1
  time = run_cmd(serial_cmd)
  print(time)
  print('**Running lockfree')
  # ./driver_output -t 1000 -n 7 -w 1000 -u 1 -d 8 -s lockfree
  time = run_cmd(serial_cmd + ['-d', str(d), '-s', 'lockfree'])
  print(time)
  # ./driver_output -t 1000 -n 7 -w 1000 -u 1 -d 8 -s [homequeue|awesome] -l [clh|mutex]
  for l, s in combo:
    print('**Running {} {}'.format(l, s))
    time = run_cmd(serial_cmd + ['-d', str(d), '-l', str(l), '-s', str(s)])
    print(time)

  print('\n### Comparing Outputs\n')
  files = [l + '_' + s + '_res.txt' for l, s in combo]
  files.append('lockfree_res.txt')

  for f in files:
    print('**Testing serial_res.txt and {}: '.format(f), end='')
    print(run_cmd(['diff', 'serial_res.txt', f]) == '')

if __name__ == '__main__':
  main()
