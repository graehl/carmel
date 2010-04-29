import os,pprint,sys,inspect
dbg=os.environ.get('DEBUG')
dbg=True

def fileline(file,line):
    if file=='<stdin>':
        raise Exception(file,line)
    f=open(file)
    for _ in range(1,line):
        r=f.readline()
    close(f)
    return r

def assertv(expected, actual, type='', message='', trans=(lambda x: x)):
    m = { '==': (lambda e, a: e == a),
          '!=': (lambda e, a: e != a),
          '<=': (lambda e, a: e <= a),
          '>=': (lambda e, a: e >= a), }
    assert m[type](trans(expected), trans(actual)), 'Expected: %s, Actual: %s, %s' % (expected, actual, message)

def assertvs(expected, actual, type='', message=''):
    assertv(expected, actual, type, message, trans=str)

def assertfail(*a):
    call=getCallString(2)
    frame=inspect.currentframe()
    frames=inspect.getouterframes(frame)
    caller=frames[2]
    cframe,cfile,cline,cfun,_,_=caller
    #srcline=fileline(cfile,cline)
    try:
        srcline=inspect.getsource(cframe)
    except IOError:
        srcline=''
    #inspect.getsource(frame)
    as=' '.join(map(str,a))
    sys.stderr.write("assertion failed: %s [%s - %s]\n"%(as,srcline,call))
    raise AssertionError(*a)

def asserta(cond,*a):
    if not cond:
        assertfail(*a)

def asserteq(a,b,*addl):
    if (a!=b):
        assertfail("a==b",a,b,*addl)
        call=getCallString(1)
        ads=' '.join(map(str,addl))
        sys.stderr.write("assertion failed: %s==%s %s (%s)"%(a,b,call,ads))
        raise AssertionError(a,b,*addl)


def assertbound(a,i,b,*r):
    if not (a<=i<b):
        assertfail("a<=i<b",a,i,b,*r)

def assertindex(i,c,*r):
    if not (0<=i<len(c)):
        assertfail("0<=i<len(c)",i,c,*r)

def assertgt(a,b,*r):
    if not a>b:
        assertfail("a>b",a,b,*r)

def x(s="debug stop."):
    raise Exception("x: "+s)
def warn(msg,pre="WARNING: "):
    sys.stderr.write(pre+str(msg)+"\n")
def flat_single(l):
    if (len(l)==1):
        return l[0]
    return l
def pstr(*l):
    return pprint.saferepr(flat_single(l))
def dumph(msg):
    if msg is not None:
        sys.stderr.write(str(msg)+": ")
def getCallString(level=1):
    #this gets us the frame of the caller and will work
    #in python versions 1.5.2 and greater (there are better
    #ways starting in 2.1
    try:
        raise FakeException("this is fake")
    except Exception, e:
        #get the current execution frame
        f = sys.exc_info()[2].tb_frame
    #go back as many call-frames as was specified
    while level >= 0:
        f = f.f_back
        level = level-1
    #if there is a self variable in the caller's local namespace then
    #we'll make the assumption that the caller is a class method
    obj = f.f_locals.get("self", None)
    functionName = f.f_code.co_name
    if obj:
        callStr = obj.__class__.__name__+"::"+f.f_code.co_name+" (line "+str(f.f_lineno)+")"
    else:
        callStr = f.f_code.co_name+" (line "+str(f.f_lineno)+")"
    return callStr
def dump(*l):
    if dbg:
        sys.stderr.write(getCallString(1)+': '+pstr(*l)+'\n')
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
         doc = getattr(item, '__doc__')
 	 doc = doc.strip()   # Remove leading/trailing whitespace.
 	 firstline = doc.split('\n')[0]
 	 print "DOC:     ", firstline


