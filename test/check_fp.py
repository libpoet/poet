#!/usr/bin/python

import os
import sys
import csv
import numpy

LOG_FILE = '/tmp/poet_log.txt'


class LogData:
  '''
    Simple class to read and extract data from poet and heartbeats log files
  '''

  def __init__(self):
    with open('/tmp/poet_log.txt', 'r') as fd:
      self.poet_data = self.parse(fd)

  def parse(self, fd):
    reader = csv.reader(fd, delimiter=' ', skipinitialspace=True)
    header = reader.next()
    indexes = {}
    data = {}
    for i in range(len(header)):
      indexes[i] = header[i]
      data[header[i]] = []

    for line in reader:
      for i in range(len(line)):
        header = indexes[i]
        data[header].append(float(line[i]))

    return data

  def get_windowed_hbr(self):
    return self.poet_data['HB_RATE']

  def graph(self):
    pass

if __name__ == '__main__':

  if len(sys.argv) < 2:
    print 'need file arg'
    exit()


  test = " ".join(sys.argv[1:])
  print test

  # make fixed point version
  os.system('make -s clean; make -s FIXED_POINT=1')

  # run fixed point version
  os.system(test)

  # read fixed point results results
  fixed_point_data = LogData()

  # make floating point version
  os.system('make -s clean; make -s')

  # run floating point version
  os.system(test)

  # read floating point results results
  floating_point_data = LogData()

  # assert that poet variables are close enough
  for variable in fixed_point_data.poet_data:
    x = fixed_point_data.poet_data[variable]
    y = floating_point_data.poet_data[variable]
    print 'Testing variable %s...' % (variable)
    try:
      numpy.testing.assert_array_almost_equal(x, y, decimal=2, verbose=True)
    except AssertionError as e:
      print e
