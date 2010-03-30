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
