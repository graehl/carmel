#!/usr/bin/env pypy
"""
TODO!
toolkit for training and evaluating distributions where conditioning context and predicted values are both opaquely identified by symbol sequences.

symbol sequences are lists of intern(s) strings
"""

def interns(xs):
    return map(intern,xs)


class Smoothing(object):
    use_counts=False # you want a hash on pred for each ctx, to count, and a total count
    use_type_counts=False # for each ctx, you want a sum of # of (types of) pred for your ctx. you can get this from len(use_counts hash)
    def __init__(self):
        self.params=[]
        self.types=dict() # just a dict (counts of types per ctx)
        self.counts=dict() # dict of dicts (each ctx has pred=>count)
    def uses_counts(self):
        return self.__class__.use_counts
    def uses_type_counts(self):
        return self.__class__.use_type_counts
    def use_params(self):
        assert(len(self.params)==0)
    def set_params(self,params=[]):
        self.params=list(params)
        self.use_params()
    def name_params(self,names=[]):
        assert(len(names)==len(params))
        self.param_names=list(names)
        self.index_param_i()
    def n_types(self,ctx):
        if self.uses_type_counts():
            return self.types[ctx]
        elif self.uses_counts():
            return len(self.counts[ctx])
        else:
            assert(0)
    def index_param_i(self):
        self.param_i=dict((i,self.param_names[i]) for i in range(0,len(self.param_names)))

class WittenBell(Smoothing):
    "WB smoothing"
    use_counts=True

class AddK(Smoothing):
    "adds a constant count > -1 to all observed events (0 counts are left as 0)"
    use_counts=True
    def use_params(self):
        self.k=float(self.params[0])

class Uniform(Smoothing):
    "equal prob for all observed events"
    use_type_counts=True

class Constant(Smoothing):
    "don't learn anything. return a non-normalized constant"
    def use_params(self):
        self.c=float(self.params[0])

smoothfactory=dict([('WB',WittenBell),('+',AddK),('U',Uniform),('CONST',Constant)])
def smoothing_args(name,*params):
    r=smoothfactory[name]()
    r.set_params(params)
    return r

import re
func_re=re.compile(r'([^(\s]+)(?:\(([^)]+)\))?')
comma_re=re.compile(r'[\s,]+')
def parse_smoothing(str):
    m=func_re.match()
    if not m:
        raise Exception("couldn't parse smoothing description %s"%str)
    name,csv=m.groups()
    if csv is not None:
        args=comma_re.split(csv)
    else:
        args=[]
    return smoothing_args(name,*args)


class Combine(object):
    def __init__(self):
        pass

class CombineProduct(Combine):
    def __init__(self):
        pass
    def probs(self,xs):
        r=1.
        for x in xs:
            r*=x
        return r

class CombineSum(Combine):
    def __init__(self):
        pass
    def probs(self,xs):
        r=0.
        for x in xs:
            r+=x
        return r

class CombineMax(Combine):
    def __init__(self):
        pass
    def probs(self,xs):
        r=0.
        for x in xs:
            if x>r:
                r=x
        return r

class CombineSingle(Combine):
    def __init__(self):
        pass
    def probs(self,xs):
        assert(len(xs)==1)
        return xs[0]

class ModelTypeTree(object):
    def __init__(self,typeid,allow_multiple):
        self.id=typeid
        self.botypes=[]
        self.combine_class=CombineProduct if allow_multiple else CombineSingle
        self.combine=(self.combine_class)()


class Model(object):
    def __init__(self):
        pass
    def parse_line(self,line):
        "return some Input object that may get cached (memory permitting). from that Input, we expect the model to make a series of predictions"
        return None

modelfactory=dict()
def register_model(cls,name=None):
    global modelfactory
    if name is None:
        name=cls.__name__
    modelfactory[name]=cls

version="0"

def backoff_main(opts):
    log("backoff-main v%s"%version)
    log(' '.join(sys.argv))
    log("options=%s"%opts)

import optfunc

@optfunc.arghelp('train','training input file here (- means STDIN)')
@optfunc.arghelp('model','name of model class')
@optfunc.arghelp('fmodel','filename for model (output if train, else input)')
@optfunc.arghelp('mode','python (or TODO: mapreduce)')
def backoff_main_opts(train='',fmodel='',model='',mode='python'):
    backoff_main(Locals())
