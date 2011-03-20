#!/usr/bin/env python2.6

import re
from graehl import *
from dumpx import *

class GraphObj(object):
    def __init__(label=None

class Graph(object):
    """
    list of labeled edges. edges attributes 'head' (to) 'tail' (from) 'reversed' and 'cost_reverse'. edges and nodes both have 'label' 'index' and 'cost'

    you can build forward and backward star indices (forward=all edges w/ given tail node; backward = w/ given head)
    , and label->edge and label->node indices. note: if you modify labels, heads, or tails, the indices will be out of date

    """
