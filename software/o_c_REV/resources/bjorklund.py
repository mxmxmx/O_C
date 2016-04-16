# The following code from http://brianhouse.net/files/bjorklund.txt
# This software is a Python implementation of the algorithm by 
# E. Bjorklund and found in his paper, 
# "The Theory of Rep-Rate Pattern Generation in the SNS Timing System." 
# https://ics-web.sns.ornl.gov/timing/Rep-Rate%20Tech%20Note.pdf
# 
# This implementation copyright (c) 2011 Brian House
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# 
# See <http://www.gnu.org/licenses/gpl.html> for details.
# 

def bjorklund(pulses, steps):
    steps = int(steps)
    pulses = int(pulses)
    if pulses > steps:
        raise ValueError    
    pattern = []    
    counts = []
    remainders = []
    divisor = steps - pulses
    remainders.append(pulses)
    level = 0
    while True:
        counts.append(divisor / remainders[level])
        remainders.append(divisor % remainders[level])
        divisor = remainders[level]
        level = level + 1
        if remainders[level] <= 1:
            break
    counts.append(divisor)
    
    def build(level):
        if level == -1:
            pattern.append(0)
        elif level == -2:
            pattern.append(1)         
        else:
            for i in xrange(0, counts[level]):
                build(level - 1)
            if remainders[level] != 0:
                build(level - 2)
    
    build(level)
    i = pattern.index(1)
    pattern = pattern[i:] + pattern[0:i]
    return pattern

# an alternative way of building Euclidean patterns, from
# the Mutable Instruments Grids code by Olivier Gillet
# https://github.com/pichenettes/eurorack/blob/master/grids/resources/lookup_tables.py

def Flatten(l):
  if hasattr(l, 'pop'):
    for item in l:
      for j in Flatten(item):
        yield j
  else:
    yield l


def EuclideanPattern(k, n):
  pattern = [[1]] * k + [[0]] * (n - k)
  while k:
    cut = min(k, len(pattern) - k)
    k, pattern = cut, [pattern[i] + pattern[k + i] for i in xrange(cut)] + \
      pattern[cut:k] + pattern[k + cut:]
  return pattern

###########
# we want to create a look-up table for use in O+C
# the following code is also adapted from 
# https://github.com/pichenettes/eurorack/blob/master/grids/resources/lookup_tables.py

for num_steps in xrange(2, 33):
  print "// %d step patterns" % num_steps
  tablerow = []
  for num_notes in xrange(0,33):
    bitmask = 0
    if (num_notes == 0) or (num_notes > num_steps):
      bitmask = 0
      pattern = [0,] * num_steps
    else:
      pattern = bjorklund(num_notes, num_steps)
      # pattern = Flatten(EuclideanPattern(num_notes, num_steps))
      i = -1
      for bit in pattern:
        i += 1
        if bit:
          bitmask |= (1 << i)
    tablerow.append(bitmask)
    print "// %2d beats" % num_notes, "".join(str(v) for v in pattern)
  j = 0
  for p in tablerow:
    j += 1
    print str(p), ",",
    if j % 4 == 0:
      print " "
# print ", ".join(str(v) for v in tablerow)
