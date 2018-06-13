#!/usr/bin/env python

"""
Program to compute the day of week for the epoch used.
"""

import datetime

print('// Day of week calculation needs to know the starting condition')
for n in range(0, 28, 4):  # Limit to multiples of 4 since that limit is in the C code
    year = 1970 + n
    d = datetime.datetime(year, 1, 1)

    if n:
        conditional = '#elif'
    else:
        conditional = '#if'
    print('%s (RTX_EPOCH - 1970) %% 28 == %d' % (conditional, n))
    print(d.strftime('const uint8_t RTCx::epochDow = %w;  // %A'))

print('#endif')