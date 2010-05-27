### from graehl import *
"""TODO:
 figure out python logging lib
"""

import sys,re,random,math

from itertools import izip

log_zero=-1e10
n_zeroprobs=0

def take(n,gen):
    i=0
    for x in gen:
        if i>=n: break
        yield x
        i+=1

def drop(n,gen):
    i=0
    for x in gen:
        if i<n:
            i+=1
            continue
        yield x

def ireduce(func, iterable, init=None):
    if init is None:
        iterable = iter(iterable)
        curr = iterable.next()
    else:
        curr = init
    for x in iterable:
        curr = func(curr, x)
        yield curr

def default_generator(gen,default=None):
    'wrap gen - if gen is None, then yield (forever) None'
    if gen is None:
        while True: yield default
    else:
        for x in gen: yield x


def close_file(f):
    if f is not sys.stdin and f is not sys.stderr and f is not sys.stdout:
        f.close()

def open_default_line(file,default=None):
    'return generator: either file lines if it can be opened, or infinitely many default'
    f=None
    try:
        f=open(file)
    except:
        while True: yield default
    for x in f: yield x

def tryelse(f,default=None):
    try:
        r=f()
    except:
        return default
    return r

def interpolate(a,b,frac_b):
    f=float(frac_b)
    return b*f+a*(1.-f)

def anneal_temp(i,ni,t0,tf):
    return interpolate(t0,tf,1 if ni<=1 else float(i)/(ni-1.))

def anneal_power(i,ni,t0,tf):
    return 1./anneal_temp(i,ni,t0,tf)

def log_prob(p):
    global n_zeroprobs
    if p==0:
        n_zeroprobs+=1
        return log_zero
    return math.log(p)

def report_zeroprobs():
    "print (and return) any zero probs since last call"
    global n_zeroprobs
    if n_zeroprobs>0:
        warn("encountered %d zeroprobs, used log(0)=%g"%(n_zeroprobs,log_zero))
    n=n_zeroprobs
    n_zeroprobs=0
    return n

def uniq_shuffled(xs):
    "doesn't preserve xs order, but removes duplicates.  fastest"
    return list(set(xs))

def uniq_stable(xs):
    'xs with duplicates removed; in original order'
    seen=set()
    r=[]
    for x in xs:
        if x not in seen:
            seen.add(x)
            r.append(x)
    return r

def uniq(xs):
    'xs must be sorted! removes duplicates'
    if len(xs)==0: return []
    p=xs[0]
    r=[p]
    for x in xs:
        if x!=p:
            r.append(x)
        p=x
    return r

def sort_uniq(xs):
    return uniq(sorted(xs))

def componentwise(xs,ys,s=sum):
    "return zs with zs[i]=s(xs[i],ys[i]); len(zs)==min(len(xs),len(ys))"
    return map(s,izip(xs,ys))

def set_agreement(test,gold):
    "return (true pos,false pos,false neg), i.e. (|test intersect gold|,|test - gold|,|gold - test|).  these can be vector_summed"
    if type(test) is not set:
        test=set(test)
    if type(gold) is not set:
        gold=set(gold)
    falsepos=len(test-gold)
    truepos=len(test)-falsepos
    falseneg=len(gold-test)
    return (truepos,falsepos,falseneg)

def pr_from_agreement(truepos,falsepos,falseneg):
    "given (true pos,false pos,false neg) return (precision,recall)"
    P=float(truepos)/(truepos+falsepos)
    R=float(truepos)/(truepos+falseneg)
    return (P,R)

def set_pr(test,gold):
    "return (precision,recall) for test and gold as sets"
    P=float(truepos)/len(test)
    R=float(truepos)/len(gold)
    return (P,R)

def fmeasure(P,R,alpha_precision=.5):
    "given precision, recall, return weighted fmeasure"
    A=float(alpha_precision)
    return 1./(A/P+(1-A)/R)

def fmeasure_str(P,R,alpha_precision=.5):
    return 'P=%.3g R=%.3g weighted(P=%g)-F=%.3g'%(P,R,alpha_precision,fmeasure(P,R,alpha_precision))

import itertools

class Alignment(object):
    apair=re.compile(r'(\d+)-(\d+)')
    def __init__(self,aline,ne,nf):
        "aline is giza-format alignment: '0-0 0-1 ...' (e-f index with 0<=e<ne, 0<=f<nf)"
        def intpair(stringpair):
            return (int(stringpair[0]),int(stringpair[1]))
        self.efpairs=list(set((intpair(Alignment.apair.match(a).group(1,2)) for a in aline.strip().split()))) if aline else []
        self.ne=ne
        self.nf=nf
    def is_identity(self):
        return self.ne==self.nf and len(self.efpairs)==self.ne and all((a,a)==b for (a,b) in itertools.izip(itertools.count(0),sorted(self.efpairs)))
    def includes_identity(self):
        if self.ne!=self.nf: return False
        s=self.efpairs_set()
        return all((i,i) in s for i in range(0,self.nf))
    def copy_blank(self):
        "return a blank alignment of same dimensions"
        return Alignment(None,self.ne,self.nf)
    def corrupt(self,p,d):
        'corrupt e and f ends of an alignment link independently in output.a with probability p; move a distorted link within +-d'
        def rdistort(p,d):
            if random.random()<p:
                return random.randint(-d,d)
            return 0
        self.efpairs=list(set((bound(e+rdistort(p,d),self.ne),bound(f+rdistort(p,d),self.nf)) for e,f in self.efpairs))
    def str_agreement(self,gold,alpha_precision=.6):
        ag=self.agreement(gold)
        p,r=pr_from_agreement(*ag)
        return Alignment.fstr(p,r,alpha_precision)
    def agreement(self,gold):
        "returns (true pos,false pos,false neg) vs. gold"
        return set_agreement(self.efpairs,gold.efpairs)
    @staticmethod
    def fstr(p,r,alpha_precision=.6):
        "alpha=.6 was best correlation w/ translation quality in Fraser+Marcu ISI-TR-616"
        return fmeasure_str(p,r,alpha_precision)
    def fully_connect(self,es,fs):
        "es and fs are lists of e and f indices, fully connect cross product"
        for e in es:
            for f in fs:
                self.efpairs.append((e,f))
    def efpairs_set(self):
        "set of (e,f) links"
        return set(self.efpairs)
    def adje(self):
        "for e word index, list of f words aligned to it"
        return adjlist(self.efpairs,self.ne)
    def adjf(self):
        "inverse of adje (indexed by f)"
        return adjlist([(f,e) for (e,f) in self.efpairs],self.nf)
    def spanadje(self):
        "minimal covering span (fa,fb) for e word index's aligned f words"
        return [span_cover_points(adj) for adj in self.adje()]
    def spanadjf(self):
        "inverse of spanadje"
        return [span_cover_points(adj) for adj in self.adjf()]
    def __str__(self):
        return " ".join(["%d-%d"%p for p in self.efpairs])

def clamp(x,a,b):
    'force x to lie on [a,b]'
    return min(b,max(a,x))

def bound(x,n):
    'force x to lie on [0,n)'
    return clamp(x,0,n-1)

def unordered_pairs(xs):
    "return list of (a,b) for all a,b in xs, such that a is before b in xs"
    n=len(xs)
    return [(xs[i],xs[j]) for i in range(0,n) for j in range(i+1,n)]
    # yuck, for x in X for y in f(x) is just like for x in X: for y in f(y): - should be backwards.

PYTHON26=sys.version >= '2.6'

# could be more concise; but hope that this is efficient.  note: on python 2.6 i saw no numerical difference when logadding 1e-15 and 1
if PYTHON26:
    def logadd(lhs,rhs):
        "return log(exp(a)+exp(b)), i.e. if a and b are logprobs, returns the logprob of their probs' sum"
    #    if a>b: return logadd(b,a)
        diff=lhs-rhs
        if diff > 36: return lhs # e^36 or more overflows double floats.
        if diff < 0: #rhs is bigger
            if diff < -36: return rhs
            return rhs+math.log1p(math.exp(diff))
        return lhs+math.log1p(math.exp(-diff))
else:
    def logadd(lhs,rhs):
        "return log(exp(a)+exp(b)), i.e. if a and b are logprobs, returns the logprob of their probs' sum"
    #    if a>b: return logadd(b,a)
        diff=lhs-rhs
        if diff > 36: return lhs # e^36 or more overflows double floats.
        if diff < 0: #rhs is bigger
            if diff < -36: return rhs
            return rhs+math.log(1.+math.exp(diff))
        return lhs+math.log(1.+math.exp(-diff))

def logadd_plus(a,b):
    return math.exp(logadd(math.log(a),math.log(b)))

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

def choosei(ps):
    "given list of possibly unnormalized probabilities, return a random index drawn from that multinomial; None if empty list input.  return with uniform prob if all ps=0 (underflow)"
    if len(ps)==0:
        return None
    z=sum(ps)
    if z<=0:
        return random.randint(0,len(ps)-1)
    c=random.random()*z
    for i in range(0,len(ps)):
        c-=ps[i]
        if c<=0:
            return i
    return len(ps)-1

def normalize_logps(logps):
    logz=reduce(logadd,logps)
    return [lp-logz for lp in logps]

def ps_from_logps(logps):
    logz=reduce(logadd,logps)
    return [math.exp(lp-logz) for lp in logps]

from dumpx import *

def choosei_logps(logps,power=1.):
    if (power!=1.):
        logps=[power*l for l in logps]
    ps=ps_from_logps(logps)
    i=choosei(ps)
#    dump(callerstring(1),i,logps,ps)
    return i

def choosei_logp(ps):
    "given list of logprobs, return random index"
    if len(ps)==0:
        return None
    z=sum(ps)
    c=random.random()*z
    for i in range(0,len(ps)):
        c-=ps[i]
        if c<=0:
            return i
    return len(ps)-1

def withp(prob):
    return random.random()<prob

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
    def __str__(self):
        return attr_str(self)

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

def log_start(s):
    sys.stderr.write("### "+s)
def log_continue(s):
    sys.stderr.write(s)
def log_finish(s):
    sys.stderr.write(s+"\n")
def log(s):
    sys.stderr.write("### "+s+"\n")

def dict_slice(d,keys):
    return dict((k,d[k]) for k in keys)

def fold(f,z,list):
    for x in list:
        z=f(z,x)
    return z

def scan(f,z,list):
    result=[z]
    for x in list:
        z=f(z,x)
        result.append(z)
    return result

#reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates ((((1+2)+3)+4)+5)
# If the optional initializer is present, it is placed before the items of the iterable in the calculation, and serves as a default when the iterable is empty
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
