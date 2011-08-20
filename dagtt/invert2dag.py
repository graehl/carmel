#!/usr/bin/env pypy
"""

"""

from graph import *

def invert_main():
    g=sample_graph()
    g.index_edges(head=False,tail=False,undir=True)
    g.clear('color',0,

import optfunc
optfunc.main(invert_main)
