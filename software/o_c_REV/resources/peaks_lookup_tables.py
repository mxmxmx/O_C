#!/usr/bin/python2.5
#
# Copyright 2014 Olivier Gillet.
#
# Author: Olivier Gillet (ol.gillet@gmail.com)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
# See http://creativecommons.org/licenses/MIT/ for more information.
#
# -----------------------------------------------------------------------------
#
# Lookup table definitions.

import numpy

"""----------------------------------------------------------------------------
LFO and envelope increments.
----------------------------------------------------------------------------"""

lookup_tables = []
lookup_tables_32 = []

sample_rate = 16666
min_frequency = 1.0 / 32.0  # Hertz
max_frequency = 160.0  # Hertz

excursion = 1 << 32
num_values = 257
min_increment = excursion * min_frequency / sample_rate
max_increment = excursion * max_frequency / sample_rate

rates = numpy.linspace(numpy.log(min_increment),
                       numpy.log(max_increment), num_values)
lookup_tables_32.append(
    ('lfo_increments', numpy.exp(rates).astype(int))
)

# Create lookup table for envelope times (x^0.25).
max_time = 8.0  # seconds
min_time = 0.0005
gamma = 0.175
min_increment = excursion / (max_time * sample_rate)
max_increment = excursion / (min_time * sample_rate)

rates = numpy.linspace(numpy.power(max_increment, -gamma),
                       numpy.power(min_increment, -gamma), num_values)

values = numpy.power(rates, -1/gamma).astype(int)
lookup_tables_32.append(
    ('env_increments', values)
)

# Create table for pitch.
a4_midi = 69
a4_pitch = 440.0
highest_octave = 116
notes = numpy.arange(
    highest_octave * 128.0,
    (highest_octave + 12) * 128.0 + 16,
    16)
pitches = a4_pitch * 2 ** ((notes - a4_midi * 128) / (128 * 12))
increments = excursion / sample_rate * pitches

lookup_tables_32.append(
    ('oscillator_increments', increments.astype(int)))


"""----------------------------------------------------------------------------
Pulse delay values.
----------------------------------------------------------------------------"""

sample_rate = 16666 / 8
min_delay = 0.001
max_delay = 10.0
gamma = 0.3
times = numpy.linspace(numpy.power(min_delay, gamma),
                       numpy.power(max_delay, gamma), num_values)
times = numpy.power(times, 1 / gamma)
lookup_tables.append(
    ('delay_times', (times * sample_rate).astype(int))
)



"""----------------------------------------------------------------------------
Gravity factors.
----------------------------------------------------------------------------"""

sample_rate = 16666
min_delay = 0.015
max_delay = 2.0
gamma = 0.2
times = numpy.linspace(numpy.power(min_delay, gamma),
                       numpy.power(max_delay, gamma), num_values)
times = numpy.power(times, 1 / gamma)
lookup_tables.append(
    ('gravity', ((times * sample_rate / (2 * 65536.0)) ** -2).astype(int))
)


"""----------------------------------------------------------------------------
Envelope curves
-----------------------------------------------------------------------------"""

env_linear = numpy.arange(0, 257.0) / 256.0
env_linear[-1] = env_linear[-2]
env_quartic = env_linear ** 3.32
env_expo = 1.0 - numpy.exp(-4 * env_linear)

violent_overdrive = numpy.tanh(8.0 * env_linear)
overdrive = numpy.tanh(5.0 * env_linear)
moderate_overdrive = numpy.tanh(2.0 * env_linear)

x = env_linear
window = numpy.exp(-x * x * 4) ** 1.5
sine = numpy.sin(8 * numpy.pi * x)
sine_fold = sine * window + numpy.arctan(3 * x) * (1 - window)
sine_fold /= numpy.abs(sine_fold).max()

lookup_tables.append(('env_linear', env_linear / env_linear.max() * 65535.0))
lookup_tables.append(('env_expo', env_expo / env_expo.max() * 65535.0))
lookup_tables.append(('env_quartic', env_quartic / env_quartic.max() * 65535.0))
lookup_tables.append(('env_tanh', moderate_overdrive / moderate_overdrive.max() * 65535.0))
lookup_tables.append(('env_sinefold', (((sine_fold / sine_fold.max()) * 65535.0) + 65535.0) / 2.0))

raised_cosine = 0.5 - numpy.cos(env_linear * numpy.pi) / 2
lookup_tables.append(('raised_cosine', raised_cosine * 65535.0))



"""----------------------------------------------------------------------------
SVF coefficients
----------------------------------------------------------------------------"""

cutoff = 440.0 * 2 ** ((numpy.arange(0, 257) - 69) / 12.0)
f = cutoff / sample_rate
f[f > 1 / 8.0] = 1 / 8.0
f = 2 * numpy.sin(numpy.pi * f)
resonance = numpy.arange(0, 257) / 257.0
damp = numpy.minimum(2 * (1 - resonance ** 0.25),
       numpy.minimum(2, 2 / f - f * 0.5))

lookup_tables.append(
    ('svf_cutoff', f * 32767.0)
)

lookup_tables.append(
    ('svf_damp', damp * 32767.0)
)

lookup_tables.append(
    ('svf_scale', ((damp / 2) ** 0.5) * 32767.0)
)