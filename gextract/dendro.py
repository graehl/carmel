from graehl import *

def IDslist(n):
    return tuple(IDs() for i in range(0,n))

def read_sparse_matrix(m,names,file):
    "last field is value, prev fields are tuple key"
    if type(file)==str:
        file=open(file)
    for line in file:
        fs=line.split()
        key=tuple(names[i][k[i]] for i in range(0,len(k)))
        m[key]=fs[-1]
    return m

def load_sparse_matrix(file):
    m=dict()
    return read_sparse_matrix(m,file)

def sparse_matrix_iternamed(m,names):
    for k,v in m.iteritems():
        yield tuple(names[i].token(k[i]) for i in range(0,len(k))),v

def sparse_to_lol(m):
    pass

import sys

def print_sparse_matrix(m,out=sys.stdout):
    for k,v in sparse_matrix_iternamed(m):
        out.write(' '.join(k)+' '+str(v)+'\n')

#def sparse_to_dense(m,s):

#'average' or 'weighted'
from matplotlib.pyplot import show
from hcluster import pdist, linkage, dendrogram
import numpy
def dendro(X,metric='cosine',combine='average',showdendro=True,leaf_label_func=id2str,**kw):
    Y = pdist(X,metric)
    Z = linkage(Y,combine)
    if showdendro:
        dendrogram(Z,leaf_label_func=leaf_label_func,**kw)
        show()
    return Z
