#!/usr/bin/env python2.6
"""

"""

from graph import *

def invert_main():
    g=Graph()
    g.edge(0,1)
    g.edge(0,2)
    g.edge(2,1)
    g.edge(1,2)
    g.write_edges(sys.stdout)
    dump(g.edges_by('head'))

import optfunc
optfunc.main(invert_main)
