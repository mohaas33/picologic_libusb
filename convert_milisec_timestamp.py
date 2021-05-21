#!/usr/bin/python

import datetime
import sys
print( datetime.datetime.fromtimestamp(float(sys.argv[1])/1000).strftime('%Y-%m-%d %H:%M:%S.%f') )
