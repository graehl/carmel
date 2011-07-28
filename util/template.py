#!/usr/bin/env python2.7
from graehl import *
from collections import defaultdict

def main(rest_=['-'],keyfields=1,sep='\t'):
    for f in rest_:
        for l in open_in(f):
            print sep.join(l.split(sep)[0:keyfields])

import optfunc
optfunc.main(main)
