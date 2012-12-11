#!/usr/bin/env python

"""log_fn.py - generate unit (function) tests automatically"""
import os
import sys
log_fn_enabled=os.environ.get('LOG_FN')
log_fn_pre="log_fn.logs/"
log_fn_post=".log_fn.py"
log_fn_indent="      "
log_fn_assertfmt="identity.assert_deep_eq(self,%s,%s)"
log_fn_default_max=os.environ.get('LOG_FN_MAX') or 10
log_fn_debug=os.environ.get('LOG_FN_DEBUG') or 0
log_fn_debug_out=sys.stderr
#log_fn_overwrite=True
#TODO: map filenames->handles, so we can overwrite on first use, and not reopen so many times

import identity

# for unit tests:

# usage:
# class MyTestCase(unittest.TestCase):
#     @assert_raise(ValueError)
#     def test_value_error(self):
#         int("A") # test succeeds
def assert_raise(exception):
    """Marks test to expect the specified exception. Call assertRaises internally"""
    def test_decorator(fn):
        def test_decorated(self, *args, **kwargs):
            self.assertRaises(exception, fn, self, *args, **kwargs)
        return test_decorated
    return test_decorator


def open_in(fname):
    "if fname is '-', return sys.stdin, else return open(fname,'r')"
    return sys.stdin if fname=='-' else open(fname,'r')

def open_out(fname,append=False):
    "if fname is '-', return sys.stdout, else return open(fname,'w').  not sure if it's ok to close stdout, so let GC close file please."
    if fname=='-':
        return sys.stdout
    return open(fname,'a' if append else 'w')

def fname(fn):
    modpre=fn.__module__
    modpre='' if (modpre is None or modpre=="__main__") else modpre+'.'
    return modpre+fn.__name__

#TODO: pass in cnamepre to decorator so we can have an open filehandle across all calls - for now we open,write,close for every call, but this isn't performance critical
default_assertfmt='assert %s == %s'
def log_fn_out(out,fn,r,args,kwargs,assertfmt=default_assertfmt):
    modpre=fn.__module__
    modpre='' if (modpre is None or modpre=="__main__") else modpre+'.'
    modpre=''
    cpre=''
    fname=fn.__name__
    cnamepre=''
    if len(args):
        a0=args[0]
        if hasattr(a0,fname) and hasattr(getattr(a0,fname),'__call__'):
            args=args[1:]
            cpre=identity.repr_default(a0)+'.'
            cnamepre=a0.__class__.__name__+'.'
    if out is None:
        out=log_fn_pre+modpre+cnamepre+fname+log_fn_post
    fout=open_out(out,True) if type(out)==str else out
    argstr=','.join(map(repr,args)+['%s=%s'%(k,repr(v)) for (k,v) in kwargs.items()])
    callstr='%s%s%s(%s)'%(modpre,cpre,fname,argstr)
    o=assertfmt%(callstr,repr(r))
    fout.write(log_fn_indent+o+'\n')
    if type(out)==str:
        fout.close()

import os,errno
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        pass
        if hasattr(exc,'errno') and exc.errno == errno.EEXIST:
            pass
        else: raise
def mkdir_parent(file):
    mkdir_p(os.path.dirname(file))

def enable_log_fn(enabled=True):
    log_fn_enabled=enabled
    if enabled:
        mkdir_parent(log_fn_pre)
enable_log_fn(log_fn_enabled)


# for generating unit tests for stateless, deterministic functions. for class methods, define __repr__ and __eq__. only log up to N non-None and N_none None returns
def log_fn(N=log_fn_default_max,N_none=log_fn_default_max,once=True,checkFunctional=True,out=None,assertfmt=None):
    if assertfmt is None: assertfmt=log_fn_assertfmt or default_assertfmt
    def log_fn_N(fn):
        n=[N,N_none]
        check=dict() if (once or check_functional) else None
        def log_fn_call(*args, **kwargs):
            r=fn(*args,**kwargs)
            if not log_fn_enabled:
                return r
            if check is not None:
                k=(fn,tuple(map(identity.identity,args)),identity.dict_identity(kwargs))
                if checkFunctional and k in check:
                    s=check[k]
                    if s!=r:
                        log_fn_debug_out.write("ERROR: call: %s\n was ->\n %s\n and is now ->\n %s\n"%(k,s,r))
                    assert r==s
                if log_fn_debug>=1:
                    log_fn_debug_out.write("call: %s\n ->\n %s\n"%(k,r))
                if once and k in check:
                    return r
                else:
                    check[k]=r
            nlog=n[1] if r is None else n[0]
            if nlog!=0:
                log_fn_out(out,fn,r,args,kwargs,assertfmt)
                if nlog>0:
                    if r is None:
                        n[1]-=1
                    else:
                        n[0]-=1
            return r
        return log_fn_call
    return log_fn_N

def assertEqual(a,b):
    assert a==b

if __name__ == "__main__":
    enable_log_fn()
    @log_fn(-1)
    def f(x,y): return 2*x+y
    class C(object):
        def __init__(self,y=5):
            self.y=y
        @log_fn(-1)
        def m(self,x):
            return f(x,self.y-x)
        __eq__ = identity.eq_by_dict
        def __repr__(self):
            return "C(%s)"%self.y
    for i in range(7):
        f(i,5-i)
        C(i).m(i)
        f(i,5-i)
        C(i).m(i)
    r=f(0,5)
    assertEqual(r,5)
    r=f(0,0)
    assertEqual(r,0)
    r=C(0).m(0)
    assertEqual(r,0)
    r=f(1,4)
    assertEqual(r,6)
    r=C(1).m(1)
    assertEqual(r,2)
    r=C(2).m(2)
    assertEqual(r,4)
    r=C(3).m(3)
    assertEqual(r,6)
    r=C(4).m(4)
    assertEqual(r,8)
    r=C().m(5)
    assertEqual(r,10)
    r=C(6).m(6)
    assertEqual(r,12)
