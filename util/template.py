#!/usr/bin/env python2.7
"""
the purpose of this script.
"""

from graehl import *
from collections import defaultdict
import optfunc

@optfunc.arghelp('rest_','input files')
def main(rest_=['-'],keyfields=1,sep='\t'):
    """-h usage"""
    for f in rest_:
        for l in open_in(f):
            print sep.join(l.split(sep)[0:keyfields])

optfunc.main(main)
