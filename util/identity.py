def ne(self,other):
    return not (self==other)

import collections



def dict_identity(h):
    return frozenset(h.iteritems())
#    return tuple(sorted(h.iteritems()))


def assert_deep_eq(self,a,b,verbose=False):
    (ai,bi)=(identity(a),identity(b))
    (ar,br)=(repr(a),repr(b))
    if verbose or not (ai==bi):
        print 'assert_deep_eq? =',ai==bi, '; by repr =',ar==br
        print ai
        print bi
    #self.assertEqual(ai,bi)
    self.assertEqual(ar,br)
    if verbose:
        print 'equal!\n'

#shallow
def make_hashable(x):
    if isinstance(x,collections.Hashable):
        return x
    if isinstance(x,collections.Sequence) or isinstance(x,list) or isinstance(x,tuple):
        return tuple(map(make_hashable,x))
    if isinstance(x,collections.Set):
        return frozenset(map(make_hashable,x))
    if isinstance(x,collections.Mapping):
        return frozenset(map(make_hashable,x.iteritems()))
    return None #ensure collision

#don't call on any loopy seq/set/map - should give meaningful ==, ignoring different ordering for mutable hash/set
#alternative: pickling?
#note: if you have a hashable collection that contains unhashable ones, this fails
def identity(x):
    if isinstance(x,tuple) and len(x)>1:
        return tuple(map(identity,x))
    if isinstance(x,collections.Hashable):
        return x
    if isinstance(x,collections.Sequence) or isinstance(x,list) or isinstance(x,tuple):
        return tuple(map(identity,x))
    if isinstance(x,collections.Set):
        return frozenset(map(identity,x))
    if isinstance(x,collections.Mapping):
        return frozenset(map(identity,x.iteritems()))
    return x

def identity_sorted(x):
    if isinstance(x,collections.Sequence):
        return tuple(sorted(map(identity,x)))
    return x

def eq_identity_sorted(x,y):
    if x is None and y is None:
        return True
    return y is not None and identity_sorted(x)==identity_sorted(y)

# exact equality (shallow)
def eq_dict(self,other,duck=True):
    if hasattr(other,'__dict__'):
        if duck or type(other) is type(self):
            return hasattr(self,'__dict__') and self.__dict__ == other.__dict__
        return False
    return self==other

def identity_eq_dict(self,other,duck=True):
    if hasattr(other,'__dict__'):
        if duck or type(other) is type(self):
            return hasattr(self,'__dict__') and identity(self.__dict__) == identity(other.__dict__)
        return False
    return self==other

def repr_default(c):
    C=type(c)
    if identity_eq_dict(C(),c):
        return C.__name__+'()'
    return repr(c)

def identity_hash(x):
    return hash(identity(x))

def identity_repr(x):
    return repr(identity(x))

def identity_hash_dict(x):
    return identity_hash(x.__dict__)

def repr_by_dict(self):
    return identity_repr(self.__dict__)

def repr_constructor(self,*args):
    return '%s(%s)'%(self.__class__.__name__,','.join(map(repr,args)))

def eq_identity(a,b):
    return identity(a)==identity(b)
# don't store as hash key until you're done mutating - not enforced! could use a flag or implement only collections.Mapping
class eqdict(dict):
    def __hash__(self):
        return hash(dict_identity(self))
    def __eq__(self,other):
        return dict_identity(self)==dict_identity(other)
    __ne__=ne
