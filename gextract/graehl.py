### from graehl import *
"""TODO:
 figure out python logging lib
"""

import sys,re,random

def choosep(p_item_list):
    "given list of tuples (p,item), return a random item according to (possibly unnormalized) p, None if empty list input"
    if len(p_item_list)==0:
        return None
    z=sum(x[0] for x in p_item_list)
    c=random.random()*z
    for (p,i) in p_item_list:
        c-=p
        if c<=0:
            return i
    return p_item_list[-1][1]



def filter2(list,p):
    "return tuple of two lists a,b: a is the subseq in list where p(a[i]) is True, b is everything else"
    a=[]
    b=[]
    for x in list:
        if p(x):
            a.append(x)
        else:
            b.append(x)
    return a,b

def func_args(func):
    "return dict func's arg=default (default=None if no default)"
    args, varargs, varkw, defaultvals = inspect.getargspec(func)
    nd=len(defaultvals)
    defaultvals = defaultvals or ()
    options = dict(zip(args[-nd:], defaultvals))
    options.pop('rest_', None)
    for a in args[0:-nd]:
        options[a]=None
    return options

class Record(object):
    def __init__(self):
        pass
    def update(self,d):
        """make fields of object for dict or object d.  FIXME: int keys in dict can't make int records (would have to hack Record to be indexable"""
        if (hasattr(d,'__dict__')):
            d=getattr(d,'__dict__')
        self.__dict__.update(d)

def getlocals(up=0):
    """returns dict of locals of calling function (or up-parent) using frame"""
    f = sys._getframe(1+up)
    args = inspect.getargvalues(f)
    return args[3]

class Locals(Record):
    def __init__(self,up=0):
        self.update(getlocals(up+1))

def object_from_dict(d):
    "setattr(obj,key)=val for key,val in d and return obj"
    obj=Record()
    obj.update(d)

def log(s):
    sys.stderr.write("### "+s+"\n")

def dict_slice(d,keys):
    return dict((k,d[k]) for k in keys)

def fold(f,z,list):
    for x in list:
        z=f(z,x)
    return z

def fold(f,z,list):
    return reduce(f,list,z)

def cartesian_product(a,b):
    "return list of tuples"
    return [(x,y) for x in a for y in b]

def range_incl(a,b):
    return range(a,b+1)

import inspect

pod_types=[int,float,long,complex,str,unicode,bool]

def attr_pairlist(obj,names=None,types=pod_types,skip_callable=True,skip_private=True):
    """return a list of tuples (a1,v1)... if names is a list ["a1",...], or all the attributes if names is None.  if types is not None, then filter the tuples to those whose value's type is in types'"""
    if not names:
        names=[a for a in map(str,dir(obj)) if not (skip_private and a[0:2] == '__')]
    #    attrs,indices=filter2(names,lambda x:type(x) is str)
    avs=[(k,getattr(obj,k)) for k in names if hasattr(obj,k)]
    return [(a,b) for (a,b) in avs if not ((skip_callable and callable(b)) or (types is not None and type(b) not in types))]
    #+[(i,obj[i]) for i in indices]

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
    return a is None or (b is not None and a[0]>=b[0] and a[1]<=b[1])

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

