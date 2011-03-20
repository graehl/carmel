#!/usr/bin/env python2.6

import re
from graehl import *
from dumpx import *
from collections import defaultdict

edge_inverse_suffix='-of'
class Edge(object):
    nextid=0
    def __init__(self,tail=None,head=None,label=None,index=None,cost=0,cost_reverse=1,reversed=False):
        if index is None:
            index=Edge.nextid
            Edge.nextid+=1
        if label is None:
            label='e'+str(index)
        self.label=label
        self.index=index
        self.cost=cost
        self.cost_reverse=cost_reverse
        self.reversed=reversed
        self.tail=tail
        self.head=head
        self.color=None
    def reverse(self):
        t=self.tail
        self.tail=self.head
        self.head=t
        c=self.cost
        self.cost=self.cost_reverse
        self.cost_reverse=c
        self.reversed = not self.reversed
    def name(self):
        return self.label+edge_inverse_suffix if self.reversed else self.label
    def __str__(self):
        return obj2str(self)
    def __repr__(self):
        return '%s(%s,%s)'%(self.name(),self.tail.label,self.head.label)#+str(self.cost)

class Vertex(object):
    nextid=0
    def __init__(self,label=None,index=None,cost=0):
        if index is None:
            index=Vertex.nextid
            Vertex.nextid+=1
        if label is None:
            label='v'+str(index)
        self.label=label
        self.index=index
        self.cost=cost
        self.color=None
    def __str__(self):
        return obj2str(self)
    def __repr__(self):
        return str(self.label)

class Vertices(dict):
    def __missing__(self,key):
        r=self[key]=Vertex(label=key)
        return r

class Graph(object):
    """
    list of labeled edges. edges attributes 'head' (to) 'tail' (from) 'reversed' and 'cost_reverse'. edges and nodes both have 'label' 'index' 'color' and 'cost'

    you can build forward and backward star indices (forward=all edges w/ given tail node; backward = w/ given head)

    optional label->edge and label->node. note: if you modify labels, heads, or tails, the indices will be out of date

    """
    def __init__(self,default_cost_reverse=1):
        self.e=[]
        self.v=Vertices()
        self.default_cost_reverse=default_cost_reverse
    def edge(self,tail,head,label=None,cost=0,cost_reverse=None):
        e=Edge(self.v[tail],self.v[head],label,len(self.e),cost,no_none(cost_reverse,self.default_cost_reverse))
        self.e.append(e)
        return e
    def __str__(self):
        return '[graph v=%s e=%s]'%(self.v.keys(),map(str,self.e))
    def write_edges(self,out):
        write_lines(map(str,self.e),out)
    def edges_by(self,attr='head'):
        r=defaultdict(list)
        for e in self.e:
            r[getattr(e,attr)].append(e)
        return r
    def index_edges(self):
        self.byhead=self.edges_by('head')
        self.bytail=self.edges_by('tail')

def graph_main():
    g=Graph()
    g.edge(0,1)
    g.edge(0,2).reverse()
    g.edge(2,1)
    g.edge(1,2)
    g.write_edges(sys.stdout)

import optfunc
optfunc.main(graph_main)

