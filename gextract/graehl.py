### from graehl import *
"""TODO:
 figure out python logging lib
"""

import sys,re

def log(s):
    sys.stderr.write("### "+s+"\n")

def fold(f,z,list):
    for x in list:
        z=f(z,x)
    return z

def reduce(f,list):
    "note: assumes f(a,b)=b if a is None"
    z=None
    for x in list:
        if z is None:
            z=x
        else:
            z=f(z,x)
    return z

def cartesian_product(a,b):
    "return list of tuples"
    return [(x,y) for x in a for y in b]

def range_incl(a,b):
    return range(a,b+1)

pod_types=[int,float,long,complex,str,unicode,bool]
def attr_pairlist(obj,names=None,types=pod_types):
    """return a list of tuples (a1,v1)... if names is a list ["a1",...], or all the attributes if names is None.  if types is not None, then filter the tuples to those whose value's type is in types'"""
    if not names:
        names=[a for a in dir(obj) if a[0:2] != '__']
    return [(k,getattr(obj,k)) for k in names if hasattr(obj,k) and (types is None or type(getattr(obj,k)) in types)]

def attr_str(obj,names=None,types=pod_types):
    "return string: a1=v1 a2=v2 for attr_pairlist"
    return ' '.join(["%s=%s"%p for p in attr_pairlist(obj,names,types)])


def open_out(fname):
    "if fname is '-', return sys.stdout, else return open(fname,'w').  not sure if it's ok to close stdout, so let GC close file please."
    if fname=='-':
        return sys.stdout
    return open(fname,'w')

def open_out_prefix(prefix,name):
    "open_out prefix+name, or stdout if prefix='-'"
    if prefix=='-':
        return sys.stdout
    return open_out(prefix+name)

def adjlist(pairs,na):
    "return adjacency list indexed by [a]=[x,...,z] for pairs (a,x) ... (a,z)"
    adj=[[] for row in xrange(na)]
    for a,b in pairs:
        adj[a].append(b)
    return adj

"""spans: empty span is None, not (0,0) or similar.  s[0]<=i<s[1] are the i in a span s."""
def span_cover_points(points):
    "returns (a,b) for half-open [a,b) covering points"
    if (len(points)==0):
        return None
    return (min(points),max(points)+1)

def span_points(s):
    if s is None: return []
    return range(s[0],s[1])

def span_empty(s):
    return s is None

def span_size(s):
    if s is None: return 0
    return s[1]-s[0]

def span_cover(sa,sb):
    """return smallest span covering both sa and sb; if either is None other is returned; 0-length spans aren't allowed - use None instead"""
    if sa is None:
        return sb
    if sb is None:
        return sa
    return (min(sa[0],sb[0]),max(sa[1],sb[1])) # 0-length spans would confuse this formula, which is why

def span_in(a,b):
    "a contained in b"
    if a is None: return True
    if b is None: return False
    return a[0]>=b[0] and a[1]<=b[1]

def span_points_except(s,points):
    "return list of points in s=[a,b) but not in list points"
    if s is None: return []
    a=s[0]
    b=s[1]
    ss=range(a,b)
    for p in points:
        if a<p<=b:
            ss[i-a]=None
    return [x for x in ss if x is not None]

def unmarked_span(s):
    return [False for x in range(s[0],s[1])]

def fresh_mark(marks,i):
    if not marks[i]:
        marks[i]=True
        return True
    return False

def span_points_fresh(span,marks):
    "if marks[i] was false for s[0]<=i<s[1], set to true and include i in return list."
    return [i for i in range(span[0],span[1]) if fresh_mark(marks,i)]

def span_mark(span,marks):
    if span is not None:
        for i in range(span[0],span[1]):
            marks[i]=True

def span_str(s):
    if s is None:
        return "[]"
    return "[%d,%d]"%s


radu_drophead=re.compile(r'\(([^~]+)~(\d+)~(\d+)\s+(-?[.0123456789]+)')
radu_lrb=re.compile(r'\((-LRB-(-\d+)?) \(\)')
radu_rrb=re.compile(r'\((-RRB-(-\d+)?) \)\)')
def radu2ptb(t):
    t=radu_drophead.sub(r'(\1',t)
    t=radu_lrb.sub(r'(\1 -LRB-)',t)
    t=radu_rrb.sub(r'(\1 -RRB-)',t)
    return t

def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)
