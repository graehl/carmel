### from graehl import *
### """TODO:
### figure out python logging lib
### """

import sys,re,random,math,os,collections,errno,time,operator,datetime

def chomp(s):
    return s.rstrip('\r\n')

def log(s,out=sys.stderr):
    out.write("### "+s+"\n")

def stripext(fname):
    return os.path.splitext(fname)[0]

basename=os.path.basename

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            pass
        else: raise

def mkdir_parent(file):
    mkdir_p(os.path.dirname(file))

import gzip

def is_terminal_fname(fname):
    "return if fname is '-' or '' or none - for stdin or stdout"
    return (fname is None) or fname=='-' or fname==''


def open_in(filename, openflags='b'):
    """if filename is '-', return sys.stdin, else return open(filename,'rb')
    (or if filename ends in .gz, gzip.open it)

    """
    if not filename: raise Exception("no input filename provided (sdllabs.io.open_in)")
    if is_terminal_filename(filename):
        return sys.stdin
    return (gzip.open if filename.endswith('.gz') else open)(filename,'r'+openflags)


def open_out(filename, append=False, mkdir=False, openflags='b'):
    """if filename is '-' or '' or none, return sys.stdout, else return open(filename,'w').
      not sure if it's ok to close stdout, so let GC close file please."""
    if not filename: raise Exception("no output filename provided (sdllabs.io.open_out)")
    if is_terminal_filename(filename):
        return sys.stdout
    if mkdir:
        mkdir_parent(filename)
    return (gzip.open if filename.endswith('.gz') else open)(filename, ('a' if append else 'w')+openflags)

##

def normalize(p_list):
    oos = sum(p for p in p_list)
    oos = 1.0/oos if sum>0 else 0
    return [p*oos for p in p_list]

def normalize_choosep(p_item_list):
    "list of tuples (p,item). return normalized-p copy"
    oos = sum(p for (p,_) in p_item_list)
    oos = 1.0/oos if sum>0 else 0
    return [(p*oos,v) for (p,v) in p_item_list]

# see http://infoweekly.blogspot.com/2011/09/follow-up-sampling-discrete.html
def compile_choosep(p_item_list):
    "list of tuples (p,item). p sums to 1. returns 'compiled' for select_choosep"
    N=len(p_item_list)
    u=1.0/N
    r=[]
    s=[x for x in p_item_list if x[0]<u] # two passes through input list or else two stacks or linked lists of slights and overfulls
    i=0
    for (p,v) in p_item_list:
        if p<u: continue
        while p>u: # eat up one s until this is empty or small too
            if i<len(s):
                ps,vs=s[i]
                i+=1
                r.append((ps*N,vs,v))
                # print '->chopped ',(ps,vs),(p,p-(u-ps),v),i,s[i-1],r[-1]
                p-=(u-ps)
            else:
                r.append((1.0,v,v))
                p-=u
        if p==u:
            # print '->full uniform',v
            r.append((1.0,v,v))
        elif p>0:
            s.append((p,v))
            # print '->small p',(p,v)
    if i<len(s):
        pr,v=s[i]
        if len(r)<N:
            # print 'remain',pr,v
            r.append((1.0,v,v))
            assert(i+1==len(s))
        else:
            assert(pr<1e-8)
    assert(len(r)==N)
    return r

def select_choosep(p01,compiled):
    if len(compiled)==0: return None
    if p01==1.0:
        return compiled[-1][1] #TOTALLY ARBITRARY - p01 should be less than 1. feel free to assert/die instead!
    assert(p01<=1.0 and p01>=0.0)
    n=len(compiled)
    pn=p01*n
    i=int(pn)
    r=pn-i # check math.modf math.floor performance
    (p1,v1,v2)=compiled[i] # if we insist on integer (positional) values then goodv is redundant (==i); new compile_choosei select_choosei etc if you like
    return v1 if r<p1 else v2

def random_choosep(compiled):
    return select_choosep(random.random(),compiled)

def withi(l):
    return [(l[i],i) for i in range(len(l))]

def withi_update(l):
    for i in xrange(len(l)):
        l[i]=(l[i],i)
    return l

def popn(l,n):
    del l[-n:]

def fill(n,z=None):
    return [z]*n

def fill2(n,m,z=None): # [n-1][m-1] = row n-1, col m-1
    return [empty(m,z) for _ in range(n)]

def invert_perm(p):
    "permutation of n items as a function from 0...n-1 to self, by what was i becomes p[i]. e.g. 0 1 2 -> 2 1 0 means p=[2 1 0]"
    dst=[None]*len(p)
    for i in xrange(len(p)):
        dst[p[i]]=i
    return dst

def compose_perm(p2,p1):
    "p2*p1=p1[p2[i]] - normal function composition. annoying. right arg gets applied first."
#    "ith element of list*p[i] = list[p[i]]; what was i becomes p1*p2[i] p1[p2[i]]; note the opposite order from usual function composition notation"
    return [p1[p2[i]] for i in xrange(len(p1))]

def commandline():
    return ' '.join(map(shell_maybe_quote,sys.argv))

def logcmd(locals=True,pre='COMMAND LINE: '):
    log(pre+commandline())
    if locals:
        loc=Locals(1)
        l=''
        if hasattr(loc,'rest_'):
            l+='arguments=%s '%(loc.rest_,)
        l+='%s'%loc
        log(l)

def percent(frac):
    return '%.2f'%(frac*100)+'%'

def percent_change(frac):

    return ('+' if frac>=0 else '')+percent(frac)

def frac_change(a,b):
    if a==0 and b==0: return 0
    return (b-a)/max(abs(a),abs(b))

def sumsq(xs):
    "sum(x*x for x in xs)"
    s=0.
    for x in xs:
        s+=x*x
    return s

def covariancec(dotprod,xsum,ysum,n):
    n=float(n)
    return (dotprod-(xsum/n)*ysum)/n

def covariance(xs,ys):
    n=len(xs)
    if len(ys)!=n:
        raise Exception("covariance of different-length lists")
    return covariancec(dotproduct(xs,ys),sum(xs),sum(ys),n)

def statc(xs):
    return (sum(xs),sumsq(xs),len(xs))

def variancec(xs,xs2,n):
    return 0. if len(xs<2) else xs2-(xs/n*xs)/float(n)

def sample_variancec(xs,xs2,n):
    return 0. if len(xs)<2 else xs2-(xs/n*xs)/(n-1.)

def variance(xs):
    return variancec(*statc(xs))

def sample_variance(xs):
    return sample_variancec(*statc(xs))

def linear_regressc(dotprod,xs,xs2,ys,n):
    b=covariancec(dotprod,xs,ys,n)/variancec(xs,xs2,n)
    a=(ys-b*xs)/n
    return (b,a)

def linear_regress(xs,ys=None):
    "return (b,a) so y=x*b+a+(error) with min sum squared error"
    if ys is None:
        xs,ys=range(0,len(xs)),xs
    n=len(xs)
    if len(ys)!=n:
        raise Exception("regression of different-length lists")
    return linear_regressc(dotproduct(xs,ys),sum(xs),sumsq(xs),sum(ys),n)
    # b=covariance(xs,ys,xsum,ysum)/variance(xs,xs2,n)
    # a=(ysum-b*xsum)/n
    # return (b,a)

def linear_regress_range(xs,ys=None):
    "generalization of max{y}-min{y}"
    return (len(xs)-1)*linear_regress(xs,ys)[0]

def linear_regress_rangec(dotprod,xs,xs2,ys,n):
    "generalization of max{y}-min{y}"
    return (n-1)*linear_regressc(dotprod,xs,xs2,ys,n)[0]

#TODO: ensure non-collision (quote w/ escape of quote-strings)

def shell_maybe_quote(s):
    if s.count("'"):
        return shellquote(s)
    return s

def shellquote(s):
    return "'" + s.replace("'", "'\\''") + "'"

def cstr_maybe_quote(s):
    e=cstr_escape(s)
    return '"%s"'%e if e!=s else s

def cstr_quote(s):
    return '"%s"'%cstr_escape(s)

def cstr_escape_nows(s):
    return s.replace('\\','\\\\').replace('"','\\"')

def cstr_escape(s):
    return cstr_escape_nows(s).replace('\n','\\n').replace('\t','\\t')

import copy

def addcp(a,b):
    r=copy.copy(a)
    r+=b
    return r

def subcp(a,b):
    r=copy.copy(a)
    r-=b
    return r

def dict_dotprod(a,b,s=0.0):
    if len(b)<len(a):
        return dict_dotprod(b,a,s)
    for k in a.iterkeys():
        if k in b:
            s+=a[k]*b[k]
    return s

def firstline(inf,default='',n=1):
    r=readfrom(inf)
    if n==1:
        for l in r:
            return l
    else:
        b=''
        for l in r:
            if n<=0:
                return b
            b+=l
            n-=1
    return default

def str2weights(s,default=0.0,f=float):
    r={}
    for w in s.split(','):
        v=w.split(':')
        if v[0] in r:
            warn('str2weights duplicate key ',v[0])
        r[v[0]]=default if len(v)==1 else f(v[1])
    return r

def dict2str(d,pairsep=',',kvsep=':',omitv=None):
    return pairsep.join(kvsep.join(x) for x in d.iteritems() if omitv is None or x[1] != omitv)

def maybestr(x):
    return '' if x is None else str(x)

def maybeint(x):
    return int(x) if len(x)>0 else None

def range2str(x):
    return '%s-%s'%(maybestr(x[0]),maybestr(x[1]))

def str2range(x):
    "range of ints [a,b], i.e. a<=x<=b, denoted a-b or -b or a- or - for unbounded above/below"
    f=x.split('-',1)
    a=maybeint(f[0])
    if len(f)==1:
        return (a,a)
    return (a,maybeint(f[1]))

def any2range(x):
    "None = infinite range"
    if x is None or isinstance(x,int): return (x,x)
    if isinstance(x,tuple) and len(x)==2: return x
    if isinstance(x,string): return str2range(x)
    return None

def any2ranges(rs):
    r=any2range(rs)
    if r is not None: return r
    return [any2range(r) for r in rs]

def inrange(x,r):
    a,b=r
    return (a is None or a<=x) and (b is None or x<=b)

def inranges(x,rs):
    return any(inrange(x,r) for r in rs)

def growlist(fill=None):
    class GrowingList(list):
        def __setitem__(self, index, value):
            if index >= len(self):
                self.extend([fill]*(index + 1 - len(self)))
            list.__setitem__(self, index, value)
    return GrowingList

def at_expand(l,index,value,fill=None):
    if index >= len(l):
        l.extend([fill]*(index + 1 - len(l)))
    list.__setitem__(l, index, value)

def indices(sequence):
    return range(len(sequence))

def iindices(sequence):
    return xrange(len(sequence))

def ival(sequence):
    return zip(range(len(sequence)), sequence)

def iival(sequence):
    return izip(xrange(len(sequence)), sequence)

def maxeq(d,k,v):
    d[k]=max(d[k],v) if k in d else v

def mineq(d,k,v):
    d[k]=min(d[k],v) if k in d else v

def reduce1eq(d,k,v,f):
    d[k]=f(d[k],v) if k in d else v

def pad(l,n,pad='',allow_over=True):
    p=len(l)<n
    if p>0:
        return l+[pad]*p
    if allow_over:
        return l
    return l[:n]

def flatlist(ll):
    r=[]
    for l in ll: r+=l
    return r

megastr=['k','m','g','t','p']
megaf=1000.
megabstr=[x_mega.lower() for x_mega in megastr]
megabf=1024.
def mega(x,dec=4,power2=False):
    suf,f=(megabstr,megabf) if power2 else (megastr,megaf)
    x=float(x)
    ps=''
    for s in suf:
        if x<f: break
        x/=f
        ps=s
    return '%.*g%s'%(dec,x,ps)

def first_in(s,xs):
    for x in xs:
        if x in s:
            return x
    return None

def any_in(s,xs):
    return first_in(s,xs) is not None


def int_equiv(x):
    return int(x) if int(x)==x else x

def no_none(x,default):
    return default if x is None else x

def entuple(*xs):
    return tuple(xs)

def diff(a,b):
    return a-b

def absdiff(a,b):
    return abs(a-b)

def dict_diff(ad,bd,f=entuple,zero=0):
    d=dict()
    for k,v in ad.iteritems():
        vb=bd.get(k,zero)
        if v!=vb:
            d[k]=f(v,vb)
    for k,v in bd.iteritems():
        if k not in ad:
            d[k]=f(zero,v)
    return d

def warn_diff(ad,bd,f=entuple,zero=0,desc='dict',descb=None,header="",post="",max=None):
    if descb is None:
        descb="%s'"%desc
    d=dict_diff(ad,bd,f=f,zero=zero)
    if len(d):
        warn('%s differences:'%header,post,max=max)
    for k,v in d.iteritems():
        warn('difference %s->%s [%s]'%(desc,descb,' '.join(k)),' = %s -> %s'%v,max=max)

def negate(x):
    return -x

def identity(x):
    return x

def sort_diff(ad,bd,key=identity,f=diff,zero=0):
    "return list of (key,diff) values sorted by key(d)"
    dd=dict_diff(ad,bd,f,zero)
    return sorted(dd.iteritems(),key=key)

def nonone(xs):
    return (x for x in xs if x is not None)

def shortest_of(strings):
    return min(strings, key=len)

def long_substr(strings):
    substr = ""
    if not strings:
        return substr
    reference = shortest_of(strings) #strings[0]
    length = len(reference)
    #find a suitable slice i:j
    for i in xrange(length):
        #only consider strings long at least len(substr) + 1
        for j in xrange(i + len(substr) + 1, length):
            candidate = reference[i:j]  # is the slice recalculated every time?
            if all(candidate in text for text in strings):
                substr = candidate
    return substr

# def subst1(s,fromsub,tosub=''):
#   i=string.find(s,fromsub)
#   if i==-1: return s
#   return s[0:i]+tosub+s[i+len(fromsub):]

# def subst1(s,fromsub,tosub=''):
#     return s.replace(fromsub,tosub,count=1)

def subst1_porch(s,substr,elide='...',porch=0):
   i=string.find(s,substr)
   if i==-1: return s
   return s[0:max(i-porch,0)]+elide+s[i+max(0,len(substr)-porch)]

def subst_greedy(strings,substr,elide='...',porch=0):
  return [subst1_porch(x,substr,elide,porch) for x in strings]

def geti_clamped(xs,i):
  if i<0:
    return xs[0]
  if i>=len(xs):
    return xs[len(xs)-1]
  return xs[i]

def crossplus(a,b,c):
  for i in xrange(0,max(len(a),len(b),len(c))):
    yield geti_clamped(a,i)+geti_clamped(b,i)+geti_clamped(c,i)

def smallest_difference(strings,elide='...',recurse=True,porch=1):
  if min(len(x) for x in strings)<=len(elide): return strings
  l=long_substr(strings)
  if l==elide: return strings
  ab=[x.split(l,1) for x in strings]
  pa=l[:porch]
  pb=l[len(l)-porch:]
  al=[a+pa for (a,_) in ab]
  bl=[b+pb for (_,b) in ab]
  if recurse:
    al=list(smallest_difference(al,elide,True,porch))
    bl=list(smallest_difference(bl,elide,True,porch))
  return crossplus(al,[elide],bl)

def common_prefix(strings):
    """ Find the longest string that is a prefix of all the strings.
    """
    if not strings:
        return ''
    prefix = strings[0]
    for s in strings:
        if len(s) < len(prefix):
            prefix = prefix[:len(s)]
        if not prefix:
            return ''
        for i in range(len(prefix)):
            if prefix[i] != s[i]:
                prefix = prefix[:i]
                break
    return prefix

from itertools import *
#from dumpx import *

#python 2.5 on
#all(x)=forall(x,None)
#any(x)=exists(x,None)

def datetoday():
    return str(datetime.datetime.today())

def datenow():
    return datetime.datetime.now()

def str2date(s):
    return datetime.datetime.strptime(s,'%Y-%m-%d %H:%M:%S.%f')

datere=re.compile(r'\d+-\d+-\d+ \d+:\d+:\d+\.\d+')

def datediff(since,now=None):
    if isinstance(since, str): since=str2date(since)
    if now is None: now=datenow()
    if isinstance(now, str): now=str2date(now)
    return now-since

def datesin(f):
    if isinstance(f, str): f=open(f)
    for line in f:
        for m in datere.findall(line):
            yield str2date(m)

def minandmax(s,l=None,u=None):
    for x in s:
        if l is None or x<l: l=x
        if u is None or x>u: u=x
    return (l,u)

def ctime(path):
    return datetime.datetime.fromtimestamp(os.path.getctime(path))
def mtime(path):
    return datetime.datetime.fromtimestamp(os.path.getmtime(path))
def filedaterange(path,usefs=True):
    mind=None
    maxd=None
    if usefs and isinstance(path, str):
        mind=ctime(path)
        maxd=mtime(path)
    return minandmax(datesin(path),mind,maxd)

def filedatespan(path,usefs=True):
    a,b=filedaterange(path,usefs)
    return b-a

def filesize(path):
    return os.stat(path).st_size

def readfrom(infile):
    return open_in(infile) if isinstance(infile, str) else infile

def pretty_float(x,digits=16):
    s='%.*g'%(digits,x)
    if digits<16:
        return s
    d=s.find('.')
    if d<0:
        return s
    else:
        e=s[-2:]
        if e=='.99' or e=='.01':
            return '%.*g'%(digits-1,x)
        return s
        # n=float(s[d+1:])
        # dump(e,s[0:d+1],n,r)
        # rounded='%0*.0'%(r,n)
        # dump(s[0:d+1],rounded,s[0:d+1]+rounded)
        # return s[0:d+1]+rounded

if __name__ == "__main__":
    ps=map(float,sys.argv[1:])
    print ps
    ps=normalize(ps)
    print ps
    pis=withi(ps)
    print pis
    c=compile_choosep(pis)
    print c
    div=5
    ns=len(ps)*div
    sn=1.0/ns
    ps=[sn*x for x in range(0,ns+1)]
    print '\n'.join('%s=>%s'%(p,select_choosep(p,c)) for p in ps)
    # print datetoday()
    # print str(str2date(datetoday()))
    # for x in ['"\\a"',"2","'",'"',r'\a\b\c\"','"a\tb\nEND"']:
    #     print x,cstr_maybe_quote(x),cstr_quote(x),cstr_escape(x),cstr_escape_nows(x)
  # for x in smallest_difference(['1e-12,1e-13,1e-14,1e14,1e15,1e-15,1e16,1e-16,1e-17,1e-18,1e18]:','1e-12,1e-13,15e-14,1e14,1e15,15e-15,1e16,15e-16,1e-17,1e-18,1e18]:']):
  #   print x

#    for f in [1e-12,1e-13,1e-14,1e14,1e15,1e-15,1e16,1e-16,1e-17,1e-18,1e18]:
#        print pretty_float(1e100*(1+f)),pretty_float(1e10*(1+f)),pretty_float(1+f),pretty_float(1-f),pretty_float(1-2*f)

class Progress(object):
    def __init__(self,max=None,out=sys.stderr,big=10000,small=None):
        self.out=open(out,'w') if isinstance(out, str) else out
        self.big=big
        self.small=big/10 if small is None else small
        self.n=0
        self.max=max
    def inc(self):
        self.n+=1
        n=self.n
        out=self.out
        if self.max is not None and n==max:
            out.write("%s\n"%n)
        elif n%self.big==0:
            out.write(str(n))
        elif n%self.small==0:
            out.write('.')
        out.flush()
    def done(self):
        self.out.write("(done, N=%s)\n"%self.n)

#    p=Progress(max=30,big=20)
#    for x in range(10,50):
#        p.inc()
#    p.done()


statfact={}
class Stat(object):
    def __init__(self,name='N'):
        self.name=name
        statfact[name]=self.__class__
    def __iadd__(self,x):
        self.count(x)
    def count(self,x):
        pass
    def pairlist(self):
        return []
    def __repr__(self):
        return '[%s: %s]'%(self.name,self.strs())
    def strs(self):
        return ' '.join(map(pairstrf,self.pairlist()))

def pairstr(p):
    return '='.join(map(str,p))

def pairstrf(p):
    return "%s=%.4g"%p

class Multistats(Stat):
    def __init__(self,name='stats',stats=None):
        Stat.__init__(self,name)
        self.stats=[] if stats is None else stats
    def count(self,x):
        Stat.count(self,x)
        for s in self.stats:
            s.count(x)
    def pairlist(self):
        return flatten(s.pairlist() for s in self.stats)

class Mean(Stat):
    def __init__(self,name='mean'):
        Stat.__init__(self,name)
        self.sum=0.
        self.N=0.
    def __iadd__(self,o):
        self.sum+=o.sum
        self.N+=o.N
    def __isub__(self,o):
        self.sum-=o.sum
        self.N-=o.N
    def count(self,x):
        self.N+=1
        self.sum+=x
    def avg(self):
        return self.sum/self.N
    def pairlist(self):
        return [('N',self.N),('mean',self.avg())]

class Variance(Mean):
    def __init__(self,name='stddev',mean=True,stddev=True,stderror=True,variance=False):
        Mean.__init__(self,name)
        self.sumsq=0.
        self.mean=mean
        self.stddev=stddev
        self.stderror=stderror
        self.variance=variance
    def __iadd__(self,o):
        super(Variance,self).__iadd__(o)
        self.sumsq+=o.sumsq
    def __isub__(self,o):
        super(Variance,self).__isub__(o)
        self.sumsq-=o.sumsq
    def sample_variance(self):
        variance=(self.sumsq-self.sum**2/self.N)/(self.N-1) if self.N>1 else 0
        if variance==0:
            return (0,0,0)
        stddev=math.sqrt(variance)
        return (variance,stddev,stddev/math.sqrt(self.N))
    def pairlist(self):
        if self.mean: r=Mean.pairlist(self)
        else: r=[('N',self.N)]
        variance,stddev,stderror=self.sample_variance()
        if self.variance: r.append(('variance',variance))
        if self.stddev: r.append(('stddev',stddev))
        if self.stderror: r.append(('stderror',stderror))
        return r
    def count(self,x):
        Mean.count(self,x)
        self.sumsq+=x*x

class Bounds(Stat):
    def __init__(self,name='bounds',min=True,max=True,N=True,index=True):
        Stat.__init__(self,name)
        self.usemin=min
        self.usemax=max
        self.index=index
        self.useN=N
        self.min=None
        self.mini=None
        self.max=None
        self.maxi=None
        self.N=0
    def count(self,x):
        if self.min is None or x<self.min:
            self.mini=self.N
            self.min=x
        if self.max is None or x>self.max:
            self.maxi=self.N
            self.max=x
        self.N+=1
    def pairlist(self):
        r=[]
        if self.useN:
            r.append(('N',self.N))
        if self.min is not None:
            r.append(('min',self.min))
            if self.index: r.append(('imin',self.mini))
        if self.max is not None:
            r.append(('max',self.max))
            if self.index: r.append(('imax',self.maxi))
        return r

class Stats(Variance):
    def __init__(self,name='stats',mean=True,stddev=True,stderror=True,variance=False,min=True,max=True,index=True):
        Variance.__init__(self,name=name,mean=mean,stddev=stddev,stderror=stderror,variance=variance)
        self.bounds=Bounds(min=min,max=max,N=False,index=index)
    def count(self,x):
        Variance.count(self,x)
        self.bounds.count(x)
    def pairlist(self):
        return Variance.pairlist(self)+self.bounds.pairlist()

class PairedDiffs(object):
    def __init__(self,d=[],statsf=Bounds,aname='a',bname='a'):
        self.ds=list(d)
        self.aname=aname
        self.bname=bname
        self.ngt=0
        self.st=statsf()
    def count(self,a,b):
        self.ds.append((a,b))
        if b>a:
            self.ngt+=1
        self.st.count(b-a)
    def __str__(self):
        return 'Paired[%s %s %s>%s]'%(self.st,self.ngt,self.bname,self.aname)

def invert_copy_dict(dst,src):
    for k,v in src.iteritems():
        dst[v]=k

def invert_dict(src):
    dst=dict()
    invert_copy_dict(dst,src)
    return dst

def invert_dictid_to_list(src):
    dst=[None]*len(src)
    invert_copy_dict(dst,src)
    return dst

class IDs(dict):
    def __init__(self,inverse=True):
        dict.__init__(self)
        self.id=0
        self.inverse=[] if inverse else None
    def __missing__(self,key):
        r=self.id
        self[key]=r
        if self.inverse is not None:
            self.inverse.append(key)
        self.id+=1
        return r
    def drop_inverse(self):
        self.inverse=None
    def cache_inverse(self):
        self.inverse=invert_dictid_to_list(self)
    def token(self,id):
        if self.inverse is None:
            self.cache_inverse()
        return self.inverse[id]

strid=IDs()
def str2id(s):
    return strid[s]

def id2str(id):
    return strid.token(id)

def append_attr(obj,attr,val):
    if hasattr(obj,attr):
        getattr(obj,attr).append(val)
    else:
        setattr(obj,attr,[val])

def take_first(a,_):
    return a

def take_second(_,b):
    return b

def disjoint_add_dict(tod,fromd,merge_fn=None,ignore_conflict=True,warn_conflict=True,desc='disjoint_add_dict'):
    """tod,fromd ; tod[k]=merge_fn(tod[k],fromd[k]) if defined, else use ignore_conflict/warn_conflict. merge_fn=None => tod[k] left alone on conflict."""
    #warn('disjoint_add %s pre-size'%desc,len(tod))
    for k,v in fromd.iteritems():
        #warn('disjoint_add %s '%desc,'%s=%s, exists-already=%s'%(k,v,k in tod),max=10)
        if k in tod:
            if merge_fn is not None:
                tod[k]=merge_fn(tod[k],v)
            elif warn_conflict or ignore_conflict:
                warn("%s disjoint_add conflict"%desc,k,max=None)
                assert(ignore_conflict)
        else:
            tod[k]=v
    #warn('disjoint_add %s post-size'%desc,len(tod))

class Stopwatch(object):
    def reset(self,clear_total=True):
        self.started=time.time()
        if clear_total: self.total=0.
    def __init__(self,name='Time'):
        self.reset(clear_total=True)
        self.name=name
    def elapsed(self):
        return 0 if self.started is None else time.time()-self.started
    def pause(self):
        self.total+=self.elapsed()
        self.started=None
    def start(self):
        self.reset(False)
    def total_elapsed(self):
        return self.elapsed()+self.total
    def __str__(self):
        e=self.total_elapsed()
        return "%s: %s s"%(self.name,e)


def typedvals(*l):
    return ' '.join(['%s:%s'%(type(x).__name__,x) for x in l])


def append_logfile(logfile,writefn=None,header='',prefix='### '):
    if logfile is not None:
        logfile=open(logfile,'a')
        logfile.write(prefix+header+"\n")
        writefn(logfile)
        logfile.write("\n\n")
        logfile.close()

def iterkeys2(a,b):
    for x in a.iterkeys():
        yield x
    for x in b.iterkeys():
        if x not in a:
            yield x

def iterlen(a):
    return sum(1.0 for _ in a)

def implies(a,b):
    if not a: return True
    return b

def forall(gen,pred=None):
    if pred is None:
        for x in gen:
            if not x:
                return False
        return True
    for x in gen:
        if not pred(x):
            return False
        return True

def exists(gen,pred=None):
    if pred is None:
        for x in gen:
            if x:
                return True
        return False
    for x in gen:
        if pred(x):
            return True
        return False

def nonempty(gen):
    "consumes an item from gen. returns True iff there was an item"
    for _ in gen:
        return True
    return False


def write_dict(d,out=sys.stdout,mappair=identity,mapk=identity,mapv=identity):
    write_tabsep(d.iteritems() if isinstance(d,dict) else d,out,mappair,mapk,mapv)

def write_tabsep(pairs,out=sys.stdout,mappair=identity,mapk=identity,mapv=identity):
    if isinstance(pairs,dict): pairs=pairs.iteritems()
    if isinstance(out, str): out=open(out,'w')
    for kv in pairs:
        #dump('tabsep',type(kv),kv)
        kv=mappair(kv)
        if isinstance(kv, tuple) and len(kv)==2:
            kv=(mapk(kv[0]),mapv(kv[1]))
        #dump('tabsep post-map',type(kv),kv)
        out.write('\t'.join(map(str,kv))+'\n')


def val_sorted(pairs,reverse=True):
    if isinstance(pairs,dict): pairs=pairs.iteritems()
    r=sorted(pairs,key=lambda x:x[1],reverse=reverse)
    #dump('val_sorted',r)
    return r

def write_nested_counts(d,out=sys.stdout):
    write_dict(d,out=out,mapv=lambda x:pairlist_str(val_sorted(x)))

def system_force(*a,**kw):
    r=os.system(*a,**kw)
    if r!=0:
        warn("FAILED: os.system(%s) EXIT=%d"%(' '.join(a),r))
        if not ('warn' in kw and kw['warn']): sys.exit(r)

def callv(l):
    #import subprocess
    #subprocess.check_call(l)
    os.system(' '.join(l))

def is_iter(x):
    try: _ = iter(x)
    except TypeError: return False
    return True

def is_nonstring_iter(x):
    return False if isinstance(x, str) else is_iter(x)

def intern_tuple(seq):
    return tuple(intern(x) for x in seq)

def firstn(l,head=None):
    return l if head is None else l[0:head]

def head_sorted(l,key=identity,reverse=False,head=None):
    return firstn(sorted(l,key=key,reverse=reverse),head=head)

def head_str(l,head=None):
    return ('All[\n' if head is None else 'First-%d\n'%head)+'\n'.join(map(str,l))+'\n'

def head_sorted_str(l,key=identity,reverse=False,head=None):
    return head_str(head_sorted(l,key=key,reverse=reverse,head=head),head=head)
#    return ('All[\n' if head is None else 'First-%d\n'%head)+'\n'.join(map(str,head_sorted(l,key=key,reverse=reverse,head=head)))+'\n'

def head_sorted_dict_val(d,reverse=False,key=identity,head=None):
    return d.iteritems() if head is None else head_sorted(d.iteritems(),reverse=reverse,key=lambda x:key(x[1]),head=head)

def head_sorted_dict_val_str(d,reverse=False,key=identity,head=None):
    return head_str(head_sorted_dict_val(d,reverse=reverse,key=key,head=head),head=head)


class RDict(dict):
    """perl's autovivification for dict of dict of ..."""
    def __missing__(self,key):
        r=self[key]=RDict()
        return r
    # def __getitem__(self, item):
    #     try:
    #         return dict.__getitem__(self, item)
    #     except KeyError:
    #         value = self[item] = type(self)()
    #         return value

class IntDict(collections.defaultdict):
    def __init__(self,*a,**kw):
        collections.defaultdict.__init__(self,int,*a,**kw)

class DictDict(collections.defaultdict):
    def __init__(self,*a,**kw):
        collections.defaultdict.__init__(self,dict,*a,**kw)

class ListDict(collections.defaultdict):
    def __init__(self,*a,**kw):
        collections.defaultdict.__init__(self,list,*a,**kw)

class FloatDict(collections.defaultdict):
    def __init__(self,*a,**kw):
        collections.defaultdict.__init__(self,float,*a,**kw)


class Histogram(object):
    """dict counting occurrences of keys; will provide sorted list of (key,count) or output to gnuplot format"""
    def __init__(self):
        self.c=dict()
        self.N=0.0 # total number of counts
    def mean(self):
        return len(self.c)/self.N
    def count(self,k,d=1):
        "by setting d to some arbitrary number, you can have any (x,y) to be graphed as a histogram or line"
        self.N+=d
        c=self.c
        if k in c:
            c[k]+=d
        else:
            c[k]=d
        assert(c[k]>=0)
        return c[k]
    def prob(self,k):
        return self.c.get(k,0.)/self.N
    def get_sorted(self):
        "return sorted list of (key,count)"
        return [(k,self.c[k]) for k in sorted(self.c.iterkeys())]
    def get_binned(self,binwidth=10):
        "assuming numeric keys, return sorted list ((min,max),count) grouped by [min,min+binwidth),[min+binwidth,min+binwidth*2) etc."
        assert binwidth is None
        assert False
    def text_gnuplot(self,binwidth=None,line=False):
        "returns a gnuplot program that uses bars if line is False, and bins if binwidth isn't None"
        assert binwidth is None
        assert line is None
        assert False
    def __str__(self):
        return str(self.get_sorted())

class LengthDistribution(object):
    def __init__(self,geometric_backoff=1.0):
        self.hist=Histogram()
        #note: shifted geometric distribution - if p(STOP)=p, then E_p[length]=1/p. this is how we will fit the geometric backoff.
        self.bo=geometric_backoff
    def count(self,length,d=1):
        self.hist.count(length,d)
    def fit_geometric(self,shifted=True): #TODO
        "return p so that the mean value is the same as histogram - https://secure.wikimedia.org/wikipedia/en/wiki/Geometric_distribution - shifted means support is 1,2,... = avg. number of trials until success. unshifted means 0,... = avg. number of failures before success"
        mean=self.hist.mean()
        assert mean is None
        assert shifted is None
        return None #

def mismatch_text(xs,ys,xname='xs',yname='ys',pre='mismatch: '):
    if xs==ys:
        return ''
    i=first_mismatch(xs,ys)
    if i<len(xs):
        if i<len(ys):
            return '%s%s[%d]!=%s[%d] ( %s != %s )'%(pre,xname,i,yname,i,xs[i],ys[i])
        else:
            return '%s%s longer than %s'%(pre,xname,yname)
    else:
        return '%s%s shorter than %s'%(pre,xname,yname)

def first_mismatch(xs,ys):
    "return first index where xs[i]!=ys[i], i.e. i may be len(xs) or len(ys) if no mismatch"
    l=min(len(xs),len(ys))
    for i in range(0,l):
        if xs[i]!=ys[i]:
            return i
    return l

def mapdictv(d,f):
    "copy of dict d but with keys v replaced by f(v), or deleted if None"
    d2=dict()
    for k,v in d.iteritems():
        v2=f(v)
        if v2 is not None:
            d2[k]=v2
    return d2

log_zero=-1e10
n_zeroprobs=0

epsilon=1e-5
neg_epsilon=-epsilon

def approx_zero(x,epsilon=epsilon):
    return abs(x)<=epsilon

def equal_or_warn(x1,x2,name='',suf1='1',suf2="2",epsilon=epsilon,pre='',post='',max=10):
    if not approx_equal(x1,x2,epsilon):
        warn("%s%s%s != %s%s"%(pre,name,suf1,name,suf2),"( %s != %s )%s"%(x1,x2,post),max=max)

# see knuth for something better, probably.  epsilon should be half of the allowable relative difference.
def approx_equal(x,a,epsilon=epsilon,absepsilon=1e-30):
    return abs(x-a) <= (absepsilon+(abs(x)+abs(a))*epsilon)

def approx_leq(x,a,epsilon=epsilon):
    return x <= a+epsilon*(abs(x)+abs(a))

def approx_geq(x,a,epsilon=epsilon):
    return x+epsilon*(abs(x)+abs(a)) >= a

def definitely_gt(x,a,epsilon=epsilon):
    return x > a+epsilon*(abs(x)+abs(a))

def definitely_lt(x,a,epsilon=epsilon):
    return x+epsilon*(abs(x)+abs(a)) < a


# scala-like itertools extensions from 9.7.2. itertools Recipes

def take(n,gen):
    "Return first n items of the iterable as a list"
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

def except_str():
    return sys.exc_info()[0]+':'+sys.exc_info()[1]

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

def open_except(f,default=None):
    return tryelse(lambda:open(f),default)

def open_first(*fs):
    for f in fs:
        if f:
            try:
                r=open(f)
            except:
                continue
            return r
    return None

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

log10_0prob=-99
def log10_prob(p):
    if p==0:
        return log10_0prob
    return math.log10(p)

def write_list(l,out=sys.stdout,name='List',header=True,after_item='\n',after_list='\n',xform=identity):
    if isinstance(out, str):
        out=open(out,'w')
    if not isinstance(l, list):
        l=list(l)
    if header:
        out.write('(N=%d) %s:%s'%(len(l),name,after_item))
    for x in l:
        out.write(str(xform(x)))
        out.write(after_item)
    out.write(after_list)
    return out

def write_lines(l,out=sys.stdout):
    if isinstance(out, str):
        out=open(out,'w')
    for x in l:
        out.write(str(x)+'\n')
    return out.name

def write_kv(l,**kw):
    write_list(l,xform=lambda x:"%s = %s"%x,**kw)

n_warn=0
warncount=IntDict()
#maxcount=set()

def warn(msg,post=None,pre="WARNING: ",max=10):
    global n_warn,warncount
    n_warn+=1
    p='\n'
    if post is not None:
        p=' '+str(post)+p
    c=warncount[msg]
    warncount[msg]=c+1
    if max is None or c<max:
        lastw=max is not None and warncount[msg]==max
        sys.stderr.write(pre+str(msg)+(" (max=%s shown; no more) "%max if lastw else "")+p)
    #if c==max: maxcount.add(msg)

def warncount_sorted():
    w=[(c,k) for (k,c) in warncount.iteritems()]
    return sorted(w,reverse=True)

def info_summary(pre='',warn_details=True,out=sys.stderr):
    if n_warn and warn_details:
        log('N\twarning',out=out)
        for w in warncount_sorted():
            log("%s\t%s"%w,out=out)
    log("%s%d total warnings"%(pre,n_warn),out=out)

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
    if not isinstance(test,set):
        test=set(test)
    if not isinstance(gold,set):
        gold=set(gold)
    falsepos=len(test-gold)
    truepos=len(test)-falsepos
    falseneg=len(gold-test)
    return (truepos,falsepos,falseneg)

def divpos_else(q,d,e):
    return e if d==0 else float(q)/d

def pr_from_agreement(truepos,falsepos,falseneg):
    "given (true pos,false pos,false neg) return (precision,recall)"
    P=divpos_else(truepos,truepos+falsepos,1.)
    R=divpos_else(truepos,truepos+falseneg,1.)
    return (P,R)

def fmeasure(P,R,alpha_precision=.5):
    "given precision, recall, return weighted fmeasure"
    A=float(alpha_precision)
    return 0. if P==0 or R==0 else 1./(A/P+(1-A)/R)

def fmeasure_str(P,R,alpha_precision=.5):
    return 'P=%.3g R=%.3g F(%.3g)=%.3g'%(P,R,alpha_precision,fmeasure(P,R,alpha_precision))

class Alignment(object):
    apair=re.compile(r'(\d+)-(\d+)')
    def __init__(self,aline,ne,nf,fe_align=False):
        "aline is giza-format alignment: '0-0 0-1 ...' (e-f index with 0<=e<ne, 0<=f<nf).  if fe_align, then aline is f-e instead.  if ne or nf is not None, bound check (should catch backwards e-f vs f-e format often)"
        def intpair(stringpair):
            a,b=(int(stringpair[0]),int(stringpair[1]))
            e,f=(b,a) if fe_align else (a,b)
            if (ne is None or e<ne) and (nf is None or f<nf):
                return e,f
            raise Exception("alignment %d-%d >= bounds %d-%d - are you reading inverse or wrong alignments? aline=%s"%(e,f,ne,nf,aline))
        self.efpairs=list(set((intpair(Alignment.apair.match(a).group(1,2)) for a in aline.strip().split()))) if aline else []
        self.ne=ne
        self.nf=nf
    def inverse(self):
        r=Alignment(None,self.nf,self.ne)
        r.efpairs=[(b,a) for (a,b) in self.efpairs]
        return r
    def is_identity(self):
        return self.ne==self.nf and len(self.efpairs)==self.ne and all((a,a)==b for (a,b) in izip(count(0),sorted(self.efpairs)))
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
        return Alignment.agreestr(ag,alpha_precision)
    def agreement(self,gold):
        "returns (true pos,false pos,false neg) vs. gold"
        return set_agreement(self.efpairs,gold.efpairs)
    @staticmethod
    def agreestr(ag,alpha_precision=.6):
        p,r=pr_from_agreement(*ag)
        return Alignment.fstr(p,r,alpha_precision)
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
        "warning: no roundtrip possible because we only print pairs and not e/f len"
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
assert(PYTHON26)
# could be more concise; but hope that this is efficient.  note: on python 2.6 i saw no numerical difference when logadding 1e-15 and 1
def logadd(lhs,rhs):
    "return log(exp(a)+exp(b)), i.e. if a and b are logprobs, returns the logprob of their probs' sum"
    diff=lhs-rhs
    if diff > 36: return lhs # e^36 or more overflows double floats.
    if diff < 0: #rhs is bigger
        if diff < -36: return rhs
        return rhs+math.log1p(math.exp(diff))
    return lhs+math.log1p(math.exp(-diff))

def log10_interp(a,b,wt_10a):
    assert(0<=wt_10a<=1)
    return log10_prob(wt_10a*10.**a+(1.-wt_10a)*10.**b)

#TODO: work even if exp(a), exp(b) out of float range
def log_interp(a,b,wt_ea):
    return math.log(wt_ea*math.exp(a)+(1.-wt_ea)*math.exp(b))

log_of_10=math.log(10)
log10_of_e=1./log_of_10
def log10tolog(e10):
    return e10*log_of_10
def logtolog10(ee):
    return ee*log10_of_e
def log_rebase(l,a,b):
    "return q such that b^q = a^l"
    return l*math.log(a)/math.log(b)
def ln_tobase(l,b):
    return l/math.log(b)
def log10_tobase(l,b):
    return l/math.log10(b)

def log10add_toe(a,b):
    return log10_of_e*logadd(log_of_10*a,log_of_10*b)
def log10add(lhs,rhs):
        "return log(exp(a)+exp(b)), i.e. if a and b are logprobs, returns the logprob of their probs' sum"
        diff=lhs-rhs
        if diff > 15: return lhs # e^36 or more overflows double floats.
        if diff < 0: #rhs is bigger
            if diff < -15: return rhs
            return rhs+math.log10(1.+10.**diff)
        return lhs+math.log(1.+10.**(-diff))

def logadd_plus(a,b):
    "for debugging logadd; not otherwise useful"
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

log_of_10=math.log(10)

def normalize_logps(logps):
    logz=reduce(logadd,logps)
    return [lp-logz for lp in logps]

def ps_from_logps(logps):
    logz=reduce(logadd,logps)
    return [math.exp(lp-logz) for lp in logps]

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
            d=d.__dict__
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
def log(s,out=sys.stderr):
    out.write("### "+s+"\n")
def logtime(s=""):
    log(time.ctime()+(": "+s if s else ""))

def sec_per(sec,N=1):
    if N is None or N<=0: return "None finished yet"
    if sec<=0: return "Infinitely many per sec (time precision too small to measure)"
    return "%g sec per"%(float(sec)/N) if sec>N else "%g per sec"%(float(N)/sec)

time_keys=dict()
def log_time_since(key="",extra="",N=1):
    t=time.time()
    if key not in time_keys:
        time_keys[key]=t
        return
    last=time_keys[key]
    time_keys[key]=t
    d=t-last
    per=' (%s, N=%s)'%(sec_per(d,N),N) if N>0 else ''
    elapsed='%s sec since last '%d
    logtime('%s%s%s%s'%(elapsed,key,extra,per))

def test_since(key="test"):
    for i in range(0,10):
        log_time_since(key,i,i)
        for _ in range(0,100000): _=1

def dict_slice(d,keys):
    return dict((k,d[k]) for k in keys)

#reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates ((((1+2)+3)+4)+5)
# If the optional initializer is present, it is placed before the items of the iterable in the calculation, and serves as a default when the iterable is empty
def fold(f,z,list):
    return reduce(f,list,z)

# def fold(f,z,list):
#     for x in list:
#         z=f(z,x)
#     return z

def scan(f,z,list):
    result=[z]
    for x in list:
        z=f(z,x)
        result.append(z)
    return result


def cartesian_product(a,b):
    "return list of tuples"
    return [(x,y) for x in a for y in b]

def range_incl(a,b):
    return range(a,b+1)

import inspect

pod_types=[int,float,long,complex,str,unicode,bool]

def attr_pairlist(obj,names=None,types=pod_types,skip_callable=True,skip_private=True,skip_trailing_underscore=True):
    """return a list of tuples (a1,v1)... if names is a list ["a1",...], or all the attributes if names is None.  if types is not None, then filter the tuples to those whose value's type is in types'"""
    if not names:
        names=[a for a in map(str,dir(obj)) if not (skip_private and a[0:2] == '__') and not (skip_trailing_underscore and a[-1:] == '_')]
    #    attrs,indices=filter2(names,lambda x:type(x) is str)
    avs=[(k,getattr(obj,k)) for k in names if hasattr(obj,k)]
    return [(a,b) for (a,b) in avs if not ((skip_callable and callable(b)) or (types is not None and type(b) not in types))]
    #+[(i,obj[i]) for i in indices]

import pprint

def attr_str(obj,names=None,types=pod_types,pretty=True,skip_trailing_underscore=True):
    "return string: a1=v1 a2=v2 for attr_pairlist"
    return pairlist_str(attr_pairlist(obj,names,types,True,True,skip_trailing_underscore),pretty)

def pairlist_str(pairlist,pretty=True):
    if pretty and len(pairlist) and len(pairlist[0])==2: pairlist=[(a,pprint.pformat(b,1,80,3)) for (a,b) in pairlist]
    return ' '.join(map(pairstr,pairlist))

def obj2str(obj,names=None,types=pod_types,pretty=True):
    return '[%s %s]'%(obj.__class__.__name__,attr_str(obj,names,types,pretty))

def writeln(line,file=sys.stdout):
    file.write(line)
    file.write('\n')


def adjlist(pairs,na):
    "return adjacency list indexed by [a]=[x,...,z] for pairs (a,x) ... (a,z)"
    adj=[[] for _ in xrange(na)]
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
            ss[p-a]=None
    return [x for x in ss if x is not None]

def unmarked_span(s):
    return [False for _ in range(s[0],s[1])]

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

import string
d2at_trans=string.maketrans('0123456789','@@@@@@@@@@')
def digit2at(s):
    return s.translate(d2at_trans)

def filename_from(s):
    r=re.sub(r'[^a-zA-Z0-9_-]','.',s)
    if r[0]=='.':
        return '_'+r[1:]
    return r

def filename_from_1to1(s):
    return filename_from(s)

digitChars=set('0123456789')
def varname_from(s):
    r=re.sub(r'[^a-zA-Z0-9_]+','_',s)
    if r[0] in digitChars:
        return '_'+r
    return r

# def take(n, iterable):
#     "Return first n items of the iterable as a list"
#     return list(islice(iterable, n))

def tabulate(function, start=0):
    "Return function(0), function(1), ..."
    return imap(function, count(start))

def consume(iterator, n):
    "Advance the iterator n-steps ahead. If n is none, consume entirely."
    # Use functions that consume iterators at C speed.
    if n is None:
        # feed the entire iterator into a zero-length deque
        collections.deque(iterator, maxlen=0)
    else:
        # advance to the empty slice starting at position n
        next(islice(iterator, n, n), None)

def nth(iterable, n, default=None):
    "Returns the nth item or a default value"
    return next(islice(iterable, n, None), default)

def quantify(iterable, pred=bool):
    "Count how many times the predicate is true"
    return sum(imap(pred, iterable))

def padnone(iterable):
    """Returns the sequence elements and then returns None indefinitely.

    Useful for emulating the behavior of the built-in map() function.
    """
    return chain(iterable, repeat(None))

def ncycles(iterable, n):
    "Returns the sequence elements n times"
    return chain.from_iterable(repeat(tuple(iterable), n))

def dotproduct(vec1, vec2):
    return sum(imap(operator.mul, vec1, vec2))

def flatten(listOfLists):
    "Flatten one level of nesting"
    return chain.from_iterable(listOfLists)

def repeatfunc(func, times=None, *args):
    """Repeat calls to func with specified arguments.

    Example:  repeatfunc(random.random)
    """
    if times is None:
        return starmap(func, repeat(args))
    return starmap(func, repeat(args, times))

def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return izip(a, b)

def grouper(n, iterable, fillvalue=None):
    "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return izip_longest(fillvalue=fillvalue, *args)

def roundrobin(*iterables):
    "roundrobin('ABC', 'D', 'EF') --> A D E B F C"
    # Recipe credited to George Sakkis
    pending = len(iterables)
    nexts = cycle(iter(it).next for it in iterables)
    while pending:
        try:
            for n in nexts:
                yield n()
        except StopIteration:
            pending -= 1
            nexts = cycle(islice(nexts, pending))

def powerset(iterable):
    "powerset([1,2,3]) --> () (1,) (2,) (3,) (1,2) (1,3) (2,3) (1,2,3)"
    s = list(iterable)
    return chain.from_iterable(combinations(s, r) for r in range(len(s)+1))

def unique_everseen(iterable, key=None):
    "List unique elements, preserving order. Remember all elements ever seen."
    # unique_everseen('AAAABBBCCDAABBB') --> A B C D
    # unique_everseen('ABBCcAD', str.lower) --> A B C D
    seen = set()
    seen_add = seen.add
    if key is None:
        for element in ifilterfalse(seen.__contains__, iterable):
            seen_add(element)
            yield element
    else:
        for element in iterable:
            k = key(element)
            if k not in seen:
                seen_add(k)
                yield element

def unique_justseen(iterable, key=None):
    "List unique elements, preserving order. Remember only the element just seen."
    # unique_justseen('AAAABBBCCDAABBB') --> A B C D A B
    # unique_justseen('ABBCcAD', str.lower) --> A B C A D
    return imap(next, imap(operator.itemgetter(1), groupby(iterable, key)))

def iter_except(func, exception, first=None):
    """ Call a function repeatedly until an exception is raised.

    Converts a call-until-exception interface to an iterator interface.
    Like __builtin__.iter(func, sentinel) but uses an exception instead
    of a sentinel to end the loop.

    Examples:
        bsddbiter = iter_except(db.next, bsddb.error, db.first)
        heapiter = iter_except(functools.partial(heappop, h), IndexError)
        dictiter = iter_except(d.popitem, KeyError)
        dequeiter = iter_except(d.popleft, IndexError)
        queueiter = iter_except(q.get_nowait, Queue.Empty)
        setiter = iter_except(s.pop, KeyError)

    """
    try:
        if first is not None:
            yield first()
        while True:
            yield func()
    except exception:
        pass

def random_product(*args, **kwds):
    "Random selection from itertools.product(*args, **kwds)"
    pools = map(tuple, args) * kwds.get('repeat', 1)
    return tuple(random.choice(pool) for pool in pools)

def random_permutation(iterable, r=None):
    "Random selection from itertools.permutations(iterable, r)"
    pool = tuple(iterable)
    if r is None: r=len(pool)
    return tuple(random.sample(pool, r))

def random_combination(iterable, r):
    "Random selection from itertools.combinations(iterable, r)"
    pool = tuple(iterable)
    n = len(pool)
    indices = sorted(random.sample(xrange(n), r))
    return tuple(pool[i] for i in indices)

def random_combination_with_replacement(iterable, r):
    "Random selection from itertools.combinations_with_replacement(iterable, r)"
    pool = tuple(iterable)
    n = len(pool)
    indices = sorted(random.randrange(n) for i in xrange(r))
    return tuple(pool[i] for i in indices)
