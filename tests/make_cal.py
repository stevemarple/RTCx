#!/usr/bin/env python

from __future__ import print_function

import argparse
import datetime
import os
import sys



os.environ['TZ'] = 'UTC'


def crprint(s):
    '''
    Print using DOS line endings (to match Arduino code)
    :param s:
    :return:
    '''
    sys.stdout.write(s)
    sys.stdout.write('\r\n')

parser = argparse.ArgumentParser(description='Make calendar')
parser.add_argument('epoch_year',
                    type=int,
                    default=1970,
                    help='Epoch year')
# parser.add_argument('end_year',
#                     type=int,
#                     default=2036,
#                     help='End year (exclusive)')

args = parser.parse_args()


# Map print to a function which always uses '\r\n' for newline, matching Arduino code.
print = crprint

unix_seconds_of_epoch = int(datetime.datetime(args.epoch_year, 1, 1).strftime('%s'))

print('# START')
print('# EPOCH: %d' % args.epoch_year)

for year in range(args.epoch_year, args.epoch_year + 68):
    for month in range(1, 13):
        d = datetime.datetime(year, month, 1)
        yday = int(d.strftime('%j'))  # Strip out leading zeros for easy comparison with Arduino output
        # Compute seconds since epoch
        unix_seconds = int(d.strftime('%s'))
        alt_epoch_seconds = unix_seconds - unix_seconds_of_epoch
        print(d.strftime('%Y-%m-%dT%H:%M:%S  ' + str(alt_epoch_seconds) + '  ' + str(yday) + '  %w  %A'))

print('# END')

