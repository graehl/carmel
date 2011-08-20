import os,pprint,sys,inspect
dbg=os.environ.get('DEBUG')
dbg=True

def callerinfo(back):
    "returns frame,file,line,fun,?,?"
    return inspect.getouterframes(inspect.currentframe())[back+1]

def callerframe(back):
    return callerinfo(back+1)[0]

def callerstring(level=1):
    f=callerframe(level+1)
    #if there is a self variable in the caller's local namespace then
    #we'll make the assumption that the caller is a class method
    obj = f.f_locals.get("self", None)
    #functionName = f.f_code.co_name
    if obj:
        callStr = obj.__class__.__name__+"::"+f.f_code.co_name+" (line "+str(f.f_lineno)+")"
    else:
        callStr = f.f_code.co_name+" (line "+str(f.f_lineno)+")"
    return callStr


def fileline(file,line):
    if file=='<stdin>':
        raise Exception(file,line)
    f=open(file)
    for _ in range(1,line):
        r=f.readline()
    f.close()
    return r

def assertv(expected, actual, type='', message='', trans=(lambda x: x)):
    m = { '==': (lambda e, a: e == a),
          '!=': (lambda e, a: e != a),
          '<=': (lambda e, a: e <= a),
          '>=': (lambda e, a: e >= a), }
    assert m[type](trans(expected), trans(actual)), 'Expected: %s, Actual: %s, %s' % (expected, actual, message)

def assertvs(expected, actual, type='', message=''):
    assertv(expected, actual, type, message, trans=str)

def assertfail(back,*a):
    framei=1+back
    call=callerstring(framei)
    #frame=inspect.currentframe()
    #frames=inspect.getouterframes(frame)
    caller=callerinfo(framei)
    cframe,cfile,cline,cfun,_,_=caller
    #srcline=fileline(cfile,cline)
    try:
        srcline=inspect.getsource(cframe)
    except IOError:
        srcline=''
    #inspect.getsource(frame)
    astr=' '.join(map(str,a))
    sys.stderr.write("assertion failed: %s [%s - %s]\n"%(astr,srcline,call))
    raise AssertionError(*a)

def asserta(cond,*a):
    if not cond:
        assertfail(1,*a)

def asserteq(a,b,*addl):
    if (a!=b):
        assertfail(1,"a==b",a,b,*addl)
        call=callerstring(1)
        ads=' '.join(map(str,addl))
        sys.stderr.write("assertion failed: %s==%s %s (%s)"%(a,b,call,ads))
        raise AssertionError(a,b,*addl)

def assertbound(a,i,b,*r):
    if not (a<=i<b):
        assertfail(1,"a<=i<b",a,i,b,*r)

def assertindex(i,c,*r):
    if not (0<=i<len(c)):
        assertfail(1,"0<=i<len(c)",i,c,*r)

cmps={ '==': (lambda e,a: e == a),
       '!=': (lambda e,a: e != a),
       '<=': (lambda e,a: e <= a),
       '>=': (lambda e,a: e >= a),
       '<':  (lambda e,a: e < a),
       '>':  (lambda e,a: e > a), }

def assertcmpi(back,a,cmp,b,*r):
    if not cmps[cmp](a,b):
        assertfail(1+back,cmp,a,b,*r)

def assertle(a,b,*addl):
    if not a<=b:
        assertfail(1,"a<=b",a,b,*addl)

def assertcmp(*r):
    assertcmpi(1,*r)

def assertgt(a,b,*r):
    assertcmpi(1,a,'>',b,*r)

def assertge(a,b,*r):
    assertcmpi(1,a,'>=',b,*r)

def assertne(a,b,*r):
    assertcmpi(1,a,'!=',b,*r)

def x(s="debug stop."):
    raise Exception("x: "+s)

def flat_single(l):
    if (len(l)==1):
        return l[0]
    return l
def pstr(*l):
    return pprint.saferepr(flat_single(l))
def dumph(msg):
    if msg is not None:
        sys.stderr.write(str(msg)+": ")
def dump(*l):
    if dbg:
        sys.stderr.write(callerstring(1)+': '+pstr(*l)+'\n')
        #pprint.pprint(flat_single(l),sys.stderr)
def dumpx(*l):
    x(pstr(*l))
def interrogate(item):
     """Print useful information about item."""
     if hasattr(item, '__name__'):
         print "NAME:    ", item.__name__
     if hasattr(item, '__class__'):
         print "CLASS:   ", item.__class__.__name__
     print "ID:      ", id(item)
     print "TYPE:    ", type(item)
     print "VALUE:   ", repr(item)
     print "CALLABLE:",
     if callable(item):
         print "Yes"
     else:
         print "No"
     if hasattr(item, '__doc__'):
         docs=item.__doc__
 	 print "DOC:     ", docs.strip().split('\n')[0]


