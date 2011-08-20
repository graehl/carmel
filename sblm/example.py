#!/usr/bin/env pypy
#-*- python -*-

#hadoop mapper for PCFG: sbmt training format tree input -> parent children\t1

version="0.1"

test=True
test_in='1000.eng-parse'
default_in=None

import os,sys
sys.path.append(os.path.dirname(sys.argv[0]))

import unittest

import tree
import optparse

from graehl import *
from dumpx import *

### main:

def main(opts):
    log("pcfg-map v%s"%version)
    log(' '.join(sys.argv))

import optfunc
@optfunc.arghelp('input','input file here (None = STDIN should be default in production)')

def options(input=default_in,test=test):
    if test:
        sys.argv=sys.argv[0:1]
        input=test_in
    main(Locals())

optfunc.main(options)

