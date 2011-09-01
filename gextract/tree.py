# tree.py
# copy from https://nlg0.isi.edu/projects/hiero/browser/branches/mira
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import re,itertools

#FIXME: not precisely reversible; we assume -LRB- and -RRB- do not appear in regular text
def paren2lrb(s):
    return s.replace('(','-LRB-').replace(')','-RRB-')

def lrb2paren(s):
    return s.replace('-LRB-','(').replace('-RRB-',')')

class Node:
    "Tree node.  length is # of nodes.  self.parent.children[self.order]=self"
    def __init__(self, label, children=None):
        self.label = label
        if children is None:
            self.children = []
        else:
            self.children = children if isinstance(children, list) else list(children)
        self.update_children()

    def update_children(self):
        i=0
        self.length = 1
        for c in self.children:
            c.parent=self
            c.order=i
            self.length += c.length
            i+=1
        self.parent = None
        self.order = 0

    def map_bigram(self,f,parent=None,left=None):
        return Node(f(self,parent,left),
                    [self.children[i].map_bigram(f,parent=self,left=(left if i==0 else self.children[i-1])) for i in range(len(self.children))])

    def reduce(self,f):
        cv=[c.reduce(f) for c in self.children]
        return f(self.label,cv)

    def visit_pcl(self,f,parent=None,leaf=True,root=True):
        "f(parent,self)"
        ch=self.children
        if len(ch) or leaf:
            if root and parent is None:
                f(None,self.label)
            elif parent is not None:
                f(parent.label,self.label)
            for c in ch:
                c.visit_pcl(f,self,leaf,root)

    def visit_lrl(self,f,leaf=True,left=None,right=None):
        "f(left,right)"
        #f(left=None,right=r,leaf=True) ... f(left=last-child,right=None,leaf=...)
        ch=self.children
        if not leaf:
            if len(ch)==0: return
            if len(ch[0].children)==0: return
        l=left
        for c in ch:
            f(l,c.label)
            c.visit_lrl(f,leaf,left,right)
            l=c.label
        f(l,right)


    def visit_pc(self,f,parent=None,leaf=True,root=True):
        if (root or parent is not None) and (leaf or len(self.children)>0):
            f(self,parent)
        for c in self.children:
            c.visit_pc(f,self,leaf,root)

    def visit_lr(self,f):
        #f(left=None,right=r) ... f(left=last-child,right=None)
        left=None
        for c in self.children:
            f(left,c)
            left=c
        f(left,None)
        for c in self.children:
            c.visit_lr(f)

    def mapnode(self,f):
        return Node(f(self),[c.mapnode(f) for c in self.children])

    def map(self,f):
        return Node(f(self.label),[c.map(f) for c in self.children])

    def relabel(self,f):
        for c in self.children:
            c.relabel(f)
        self.label=f(self.label)

    def relabelnode(self,f):
        for c in self.children:
            c.relabelnode(f)
        self.label=f(self)

    def map_skipping_node(self,l,f):
        n=Node(l)
        self.map_skipping_children(f,n.children)
        n.update_children()
        return n

    def map_skipping_outlist(self, f, outlist):
        "append to outlist trees where label=f(t.label) is not None"
        l=f(self.label)
        if l is None:
            self.map_skipping_children(f,outlist)
        else:
            outlist.append(self.map_skipping_node(l,f))

    def map_skipping_children(self,f,outlist):
        for c in self.children:
            c.map_skipping_outlist(f,outlist)

    def map_skipping(self, f):
        "return t with t.label=f(self.label) - may be None, and children replaced by map_skipping_list"
        return self.map_skipping_node(f(self.label),f)

    def map_skipping_list(self,f):
        r=[]
        self.map_skipping_outlist(f,r)
        return r

    def intern(self):
        self.label=intern(self.label)
        for c in self.children:
            c.intern()

    def is_top(self):
        'is self TOP(X(...)) for some TOP,X?'
        return self is not None and self.parent is None and len(self.children)==1

    def is_s(self):
        'is parent a top?  then this is its single descendant'
        return self.parent.is_top()

    def __len__(self):
        "length of frontier"
        return self.length

    def size(self):
        "number of nodes"
        return 1+sum(c.size() for c in self.children)

    def get(self,gornaddr):
        """gornaddr is a list [] for root [0] for first child of root, [0,2] for 3rd child of that, etc.
        was __getitem__ but too valuable to discover mistaken use of tree as tuple/list, so new name"""
        if len(gornaddr) == 0:
            return self
        return self.children[gornaddr[0]][gornaddr[1:]]

    def filter_inplace(self,keep):
        "top down, remove subtree with root node t unless keep(t).  returns new tree (destructive update of old one)"
        if not keep(self):
            return None
        return self.filter_inplace_children(keep)

    def filter_inplace_children(self,keep):
        "like filter_inplace, but always keep root"
        newc=None
        oldc=self.children
        newi=0
        for i in range(0,len(oldc)):
            c=oldc[i]
            kc=c.filter(keep)
            if kc is None:
                self.length -= 1
                if newc is None:
                    newc=oldc[0:i]
                    newi=i
            else:
                if newc is not None:
                    newc.append(c)
                    c.order=newi
                newi+=1
        if newc is not None:
            self.children=newc
        return self

    def filter(self,keep):
        "like filter_inplace, but doesn't destroy original"
        if not keep(self):
            return None
        return self.filter_children(keep)

    def filter_children(self,keep):
        "like filter, but always keep root"
        return Node(self.label,filter(lambda x:x is not None,(c.filter(keep) for c in self.children)))

    def zip_postorder(self,other):
        "other and self have the same shape; yield tuples (selfnode,othernode) in postorder"
        for c,oc in zip(self.children,other.children):
            for p in c.zip_postorder(oc):
                yield p
        yield (self,other)

    def set_attr_from(self,srct,dest_attr,src_attr='label',default=None,skip_missing=False):
        "setattr(node,dest_attr,getattr(srctnode,src_attr)) for every node in self, and every srctnode in srct.  if a srct node is missing the attribute, use default, or don't set the attribute at all if skip_missing"
        if hasattr(srct,src_attr):
            setattr(self,dest_attr,getattr(srct,src_attr))
        elif not skip_missing:
            setattr(self,dest_attr,default)
        for c,sc in zip(self.children,srct.children):
            c.set_attr_from(sc,dest_attr,src_attr,default,skip_missing)

    def find_ancestor(self,pred):
        "return closest ancestor such that pred(ancestor), or else None"
        p=self.parent
        while p is not None and not pred(p):
            p=p.parent
        return p

    def find_descendants(self,pred,r):
        "return list of frontier of descendants such that pred(descendant).  frontier not meaning just leaves - internal nodes with pred(node) stop recursion"
        if r is None:
            r=[]
            for c in self.children: c.find_descendants(pred,r)
            return r
        else:
            if pred(self):
                r.append(self)
            else:
                for c in self.children: c.find_descendants(pred,r)

    def __copy__(self):
        return self.relabel(lambda x:x.label)
        #return Node(self.label,[c.__copy__() for c in self.children])

    def relabel(self,newlabel):
        "copy w/ t'.label = newlabel(t)"
        return Node(newlabel(self),[c.relabel(newlabel) for c in self.children])

    def yield_labels(self):
        return [t.label for t in self.frontier()]

    def __str__(self):
        return self.str(radu=False)

    def __repr__(self):
        return self.str()

    def str(self,radu=False,square=False):
        return self.str_impl(radu_paren=radu,radu_head=radu,radu_prob=radu,brackets="[]" if square else "()")

    def str_impl(self,radu_paren=False,radu_head=False,radu_prob=False,brackets="()",lrb=True):
        if len(self.children) != 0:
            l=str(self.label)
            s = brackets[0] + (l if (radu_paren or not lrb) else paren2lrb(l))
            nonterm=len(self.children)>1 or len(self.children[0].children)!=0
            if radu_head and nonterm: s+='~0~0'
            if radu_paren: s += ' '
            if radu_prob and nonterm: s+='0.0 '
            for child in self.children:
                if not radu_paren: s += ' '
                s += child.str_impl(radu_paren,radu_head,radu_prob,brackets,lrb)
            s += brackets[1]
            if radu_paren: s += ' '
            return s
        else:
            s = str(self.label)
            if not radu_paren:
                return paren2lrb(s) if lrb else s
            return s

    def descendant(self, addr):
        if len(addr) == 0:
            return self
        else:
            return self.children[addr[0]].descendant(addr[1:])

    def append_child(self, child):
        child.parent=self
        i=len(self.children)
        self.children.append(child)
        self.children[i]=i
        if len(self.children)>1:
            self.length+=child.length
        else:
            self.length=child.length

    def insert_child(self, i, child):
        child.parent = self
        self.children[i:i] = [child]
        for j in range(i,len(self.children)):
            self.children[j].order = j
        if len(self.children) > 1:
            self.length += child.length
        else:
            self.length = child.length # because self.label changes into nonterminal

    def delete_child(self, i):
        self.children[i].parent = None
        self.children[i].order = 0
        self.length -= self.children[i].length
        if i != -1:
            self.children[i:i+1] = []
        else:
            self.children[-1:] = []
        for j in range(i,len(self.children)):
            self.children[j].order = j

    def detach(self):
        self.parent.delete_child(self.order)

    def delete_clean(self):
        "Cleans up childless ancestors"
        parent = self.parent
        self.detach()
        if len(parent.children) == 0:
            parent.delete_clean()

    def frontier(self):
        if len(self.children) != 0:
            l = []
            for child in self.children:
                l.extend(child.frontier())
            return l
        else:
            return [self]

    def is_dominated_by(self, node):
        return self is node or (self.parent != None and self.parent.is_dominated_by(node))

    def dominates(self, arg):
        if isinstance(arg, list):
            flag = 1
            for node in arg:
                flag = flag and node.is_dominated_by(self)
            return flag
        elif isinstance(arg, Node):
            return node.is_dominated_by(self)

    def nodes_byspan(self):
        """return a hash which maps from spans to lists of nodes (top-down)."""
        s = {}

        def visit(node, i):
            s.setdefault((i,i+node.length), []).append(node)
            for child in node.children:
                visit(child, i)
                i += child.length

        visit(self, 0)

        return s

    def spans_bynode(self):
        """return a hash which maps from nodes to spans"""
        spans = {}
        nj = 0
        for node in self.postorder():
            if node.is_terminal():
                spans[node] = (nj,nj+1)
                nj += 1
            else:
                spans[node] = (spans[node.children[0]][0], spans[node.children[-1]][1])
        return spans

    def all_children(self):
        for child in self.children:
            for desc in child.preorder():
                yield desc

    def preorder(self):
        yield self
        for child in self.children: # inlining of all_children for efficiency
            for desc in child.preorder():
                yield desc

    def postorder(self):
        for child in self.children:
            for desc in child.postorder():
                yield desc
        yield self

    def is_terminal(self):
        return len(self.children) == 0

    def is_preterminal(self):
        return len(self.children) == 1 and self.children[0].is_terminal()

    def is_cat(self):
        return not (self.is_terminal() or self.is_preterminal())

    def size_cat(self):
        if self.is_terminal() or self.is_preterminal(): return 0
        return 1+sum(c.size_cat() for c in self.children)

    def size_nts(self):
        if self.is_terminal(): return 0
        return 1+sum(c.size_nts() for c in self.children)

    def label_lrb(self):
        return paren2lrb(str(self.label))

def scan_tree(tokens, pos, paren_after_root=False):
    try:
        if tokens[pos] == ")":
            return (None, pos)
        if paren_after_root:
            if tokens[pos]=='(':
                label=''
                pos+=1
            else:
                label=lrb2paren(tokens[pos])
                if tokens[pos+1]=='(':
                    pos+=2
                else:
                    return (Node(label,[]),pos+1)
        else:
            if tokens[pos] == "(":
                if tokens[pos+1] == "(":
                    label = ""
                    pos += 1
                else:
                    label = lrb2paren(tokens[pos+1])
                    pos += 2
            else:
                label = lrb2paren(tokens[pos])
                return (Node(label,[]), pos+1)
        children = []
        while True:
            (child, pos) = scan_tree(tokens, pos, paren_after_root)
            if child is None: break
            children.append(child)
        if tokens[pos] == ")":
            return (Node(label, children), pos+1)
        else:
            return (None, pos)

    except IndexError:
        return (None, pos)

tokenizer = re.compile(r"\(|\)|[^()\s]+")

def str_to_tree(s,paren_after_root=False):
    toks=tokenizer.findall(s)
    if len(toks)>2 and toks[0] == '(' and toks[1]=='(' and toks[-2]==')' and toks[-1]==')': #berkeley parse ( (tree) )
        toks=toks[1:-1]
    (tree, n) = scan_tree(toks,0,paren_after_root)
    return tree


