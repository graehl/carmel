from graehl import *

def nIDs(n=2):
    return tuple(IDs() for i in range(0,n))

def read_sparse_matrix(m,names,file='up.cluster.eng',val=float):
    "last field is value, prev fields are tuple key"
    if type(file)==str:
        file=open(file)
    for line in file:
        fs=line.split()
        key=tuple(names[i][fs[i]] for i in range(0,len(fs)-1))
        m[key]=val(fs[-1])
    return m

def load_sparse_matrix(file):
    m=dict()
    return read_sparse_matrix(m,file)

def sparse_to_dense(m,names):
    nr,nc=map(len,names)
    ret=[[0]*nc for _ in range(0,nr)]
    for k,v in m.iteritems():
        r,c=k
        ret[r][c]=v
    return ret

def sparse_matrix_iternamed(m,names):
    for k,v in m.iteritems():
        yield tuple(names[i].token(k[i]) for i in range(0,len(k))),v

def sparse_to_lol(m):
    pass

import sys

def print_sparse_matrix(m,names,out=sys.stdout):
    for k,v in sparse_matrix_iternamed(m,names):
        out.write(' '.join(k)+' '+str(v)+'\n')

#def sparse_to_dense(m,s):

#'average' or 'weighted'
from matplotlib.pyplot import show
from hcluster import pdist, linkage, dendrogram
import numpy
from scipy import sparse
def dendro(X,metric='cosine',combine='average',showdendro=True,leaf_label_func=identity,**kw):
    Y = pdist(X,metric)
    Z = linkage(Y,combine)
    if showdendro:
        dendrogram(Z,leaf_label_func=leaf_label_func,**kw)
        show()
    return Z

def test(file='up.cluster',m=None,n=50000):
    if m is None:
        m=dict()
    names=nIDs()
    read_sparse_matrix(m,names,file=file)
    #print_sparse_matrix(m,names)
    d=sparse_to_dense(m,names)
    dendro(d,leaf_label_func=lambda x:names[0].token(x),leaf_font_size=10)

