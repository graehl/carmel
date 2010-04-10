import os,pprint,sys
def x(s="debug stop."):
    raise Exception("x: "+s)
def warn(msg,pre="WARNING: "):
    sys.stderr.write(pre+str(msg)+"\n")
def flat_single(l):
    if (len(l)==1):
        return l[0]
    return l
dbg=os.environ.get('DEBUG')
dbg=True
def pstr(*l):
    return pprint.saferepr(flat_single(l))
def dump(*l):
    if dbg:
        sys.stderr.write(pstr(*l)+'\n')
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
