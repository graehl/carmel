#!/usr/bin/env pypy

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
    def other_v(self,v):
        "return (u,cost) such that v->u or u->v (reversed cost)."
        if self.tail==v:
            return (self.head,self.cost)
        return (self.tail,self.cost_reverse)
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
    def register(self,v):
        self[v.label]=v

def dfs(v,adj,visit_pre=None,visit_post=None,yield_pre=False,yield_post=False):
    """pre: all reachable v color were false before recursing with dfs. post: yields visited nodes in """
    if v.color: return
    yield_either=yield_pre or yield_post
    if yield_pre: yield v
    if visit_pre: visit(v)
    v.color=1
    for e in adj[v]:
        u,c=e.other_v(v)
        if u.color: continue
        if yield_either:
            for v in dfs(u,adj,visit_pre,visit_post,yield_pre,yield_post):
                yield v
        else:
            dfs(u,adj,visit_pre,visit_post,yield_pre,yield_post)
    v.color=2
    if yield_post: yield v
    if visit_post: visit(v)

class Graph(object):
    """
    list of labeled edges. edges attributes 'head' (to) 'tail' (from) 'reversed' and 'cost_reverse'. edges and nodes both have 'label' 'index' 'color' and 'cost'

    you can build forward and backward star indices (forward=all edges w/ given tail node; backward = w/ given head)

    optional label->edge and label->node. note: if you modify labels, heads, or tails, the indices will be out of date

    """
    def clear(self,attr,val=None,e=True,v=True):
        if e:
            for e in self.e:
                setattr(e,attr,val)
        if v:
            for v in self.v:
                setattr(v,attr,val)
    def __init__(self,default_cost_reverse=1):
        self.e=[]
        self.v=Vertices()
        self.default_cost_reverse=default_cost_reverse
    def vertices(self):
        return self.v.values()
    def edge(self,tail,head,label=None,cost=0,cost_reverse=None):
        e=Edge(self.v[tail],self.v[head],label,len(self.e),cost,no_none(cost_reverse,self.default_cost_reverse))
        self.e.append(e)
        return e
    def index_vertices(self):
        """needed if you didn't add using self.v or self.edge()"""
        for e in self.e:
            self.v.register(e.head)
            self.v.register(e.tail)
    def __str__(self):
        return '[graph v=%s e=%s]'%(self.v.keys(),map(str,self.e))
    def write_edges(self,out):
        write_lines(map(str,self.e),out)
    def edges_by(self,*attrs):
        r=defaultdict(list)
        for e in self.e:
            for attr in attrs:
                r[getattr(e,attr)].append(e)
        return r
    def adj(self,adjname,*attrs):
        for e in self.e:
            for attr in attrs:
                append_attr(getattr(e,attr),adjname,e)
    def index_edges(self,head=True,tail=True,undir=True):
        if head: self.byhead=self.edges_by('head')
        if tail: self.bytail=self.edges_by('tail')
        if undir: self.undir=self.edges_by('head','tail')

def sample_graph():
    g=Graph()
    g.edge(0,1)
    g.edge(0,2).reverse()
    g.edge(2,1)
    g.edge(1,2)
    print list(g.dfs(all=True,start=0))

def graph_main():
    g=sample_graph()
    g.write_edges(sys.stdout)

import optfunc
optfunc.main(graph_main)

