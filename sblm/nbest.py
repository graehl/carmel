#!/usr/bin/env pypy
from graehl import *
from dumpx import *
from tree import *
from etree import *

import re

lrb_re=re.compile(r'(\(-LRB-(?:-\d+) )\(\)')
rrb_re=re.compile(r'(\(-RRB-(?:-\d+) )\)\)')
def lrb_repl(m):
    return m.group(1)+'-LRB-)'
def rrb_repl(m):
    return m.group(1)+'-RRB-)'
def rb_esc(s):
    return lrb_re.sub(lrb_repl,rrb_re.sub(rrb_repl,s))
def nbest_tree(s):
    s=rb_esc(s)
    t=str_to_tree_warn(s)
    if t is not None:
        for x in t.preorder():
            if not x.is_terminal():
                x.label=tree.paren2lrb(x.label)
        #    t.relabelnode(lambda x:tree.paren2lrb(x.label) if x.is_cat() else x.label)
    return t

nbesthstr='NBEST '
numres=r'[+\-]?(?:\.\d+|\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)'
numre=re.compile(numres)
fieldeqres=r'(?:\s|^)(\S+)='
numfieldres=fieldeqres+'('+numres+')'
numfieldre=re.compile(numfieldres)
bracketres=r'\[([^\]]*)\]'
bracketre=re.compile(bracketres)
bracefieldres=r'{{{(.*?)}}}'
def bracefieldre(f):
  return re.compile(r' %s=%s'%(f,bracefieldres))
#use ' ' because \b doesn't work because of foreign-tree \btree

indescs=[('^','^^'), ('[','^{'),(']','^}'),(',','^/'),(':','^;'),('<','^('),('>','^)')]
indescsr=reversed(indescs)
featescs=set(k for (k,_) in indescs)-set(['[',']','^'])
def needsesc(n):
    return first_in(featescs,n)

def escape_indicator(x):
    for (a,b) in indescs:
        x=x.replace(a,b)
    return x
def unescape_indicator(x):
    for (a,b) in indescs:
        x=x.replace(b,a)
    return x

def getfield_brace(f,s,single=True):
  "single -> return single match else None (even if multiple matches); otherwise return list of matches"
  res=bracefieldre(f)
  l=list(res.findall(s))
  if single:
    if len(l)!=1:
      raise Exception("got %s copies of %s={{{...}}}; wanted 1, in %s"%(len(l),f,s))
    return l[0]
  return l

def getfields_num(s):
    return [(n,int_equiv(float(v))) for (n,v) in numfieldre.findall(s)]

def nbests(ns):
    for l in ns:
        if l.startswith(nbesthstr):
            yield (l,getfields_num(l))

#may return int or float! this may cause bugs if you don't expect it
def yieldfields_num(s):
    for (n,v) in numfieldre.findall(s):
        yield (n,float(v))


def getfields_float(s):
    return [(n,float(v)) for (n,v) in numfieldre.findall(s)]

def inds(f,pre):
    pre+='['
    if isinstance(f,str): f=getfields_num(f)
    d=dict()
    for n,v in f:
        if n.startswith(pre):
            a=bracketre.findall(n)
            if len(a)>=0:
                d[tuple(map(unescape_indicator,a))]=v
    return d

def indre(pre):
    res=r' (?:%s)\[\S*\]=%s'%(pre,numres)
    return re.compile(res)

def stripinds(pre,nbeststr):
    return indre(pre).sub('',nbeststr)

def stripnumfeat(k,nbeststr):
    return re.sub(r' %s=%s'%(k,numres),'',nbeststr)

def ind_quote(k):
    return ''.join('['+escape_indicator(x)+']' for x in k)

def strind(pre,k,v):
    return '%s%s=%s'%(pre,ind_quote(k),v)

def strinds(pre,inds):
    return ' '.join(strind(pre,k,v) for (k,v) in inds.iteritems())

if __name__ == "__main__":
    r=re.compile(numres)
    for x in []: #'a3.4 -5e-10 .1 0 1 1. -3e+40 5e10']:
        print r.findall(x)
    ns='''
    NBEST sent=1176 nbest=1 totalcost=-42.1689 hyp={{{the number of the price to be fit and proper , " the spokesman said ?}}} tree={{{(TOP (S-1 (S-C-0 (NP-C-0 (NPB-0 (DT-1 the) (NN-2 number)) (PP-0 (IN-0 of) (NP-C-1 (NPB-0 (DT-1 the) (NN-2 price))))) (VP-1 (TO-0 to) (VP-C-1 (VB-1 be) (ADJP-0 (JJ-0 fit) (CC-0 and) (JJ-1 proper))))) (,-0 ,) (''-0 ") (NP-C-0 (NPB-0 (DT-1 the) (NN-2 spokesman))) (VP-2 (VBD-0 said)) (.-0 ?)))}}} derivation={{{(1010100014 (429693942 (444170192 (234254523 199036167 285550478) 185165479) 129596))}}} binarized-derivation={{{([inf,-42.1689][0,5]1010100014 ([-38.4264,-41.2364][1,5]429693942 ([-19.4377,-22.663][1,4]444170192 ([-1.37713,-3.36647][1,3]234254523 [7.20957,4.75075][1,2]199036167 [-1.04554,-2.8439][2,3]285550478 ) [-2.1986,-5.45431][3,4]185165479 ) [-1.5258,-4.33167][4,5]129596 ) )}}} foreign-tree={{{}}} used-rules={{{129596 185165479 199036167 234254523 285550478 429693942 444170192 1010100014}}} sbtm-cost=-51.451 text-length=16 count=33817 corpora-LDC2005T10=0.85236 corpora-LDC2006E24=0.155029 corpora-LDC2006E34=0.0313194 corpora-LDC2006E26=0.00557431 corpora-LDC2005E83=0.0073335 corpora-LDC2006E92=0.0926128 corpora-LDC2004T08_HK_News=0.611677 corpora-LDC2007E87=0.163146 corpora-LDC2006E85=0.0262587 corpora-LDC2008G06=0.103836 corpora-LDC2007E06=0.015181 corpora-LDC2006E93=0.0361409 corpora-LDC2007E101=0.357006 corpora-LDC2006G05=1.17792 corpora-LDC2008E40=0.0207818 corpora-LDC2007E103=3.22112 genre-web=0.0941518 genre-bn=0.275816 genre-wl=0.013862 genre-bc=0.311055 genre-nw=6.26691 n_var=6 comma_nodes=1 comma_insertion=1 count_category_5=5 trivial_cond_prob=5.79757 lef.stem=23.8061 phrase_pfe=7.97642 gt_prob=33.7002 model1nrm=24.376 lfe.stem=6.18636 model1inv=5.46316 nomodel1inv=3 is_lexicalized=7 phrase_pef=18.2271 taglex.nrm=23.46 derivation-size=8 corpora-LDC2005T06=0.0138693 corpora-LDC2008G05=0.0506064 genre-ng=0.0150507 adjp_nodes=1 corpora-LDC2003E07=0.0231563 genre-treebank=0.0231563 cc_nodes=1 corpora-LDC2007E46=0.0311577 dt_nodes=3 the_insertion=2 to_nodes=1 to_insertion=1 corpora-LDC2006E86=0.00391652 in_nodes=1 of_insertion=1 jj_nodes=2 nn_nodes=3 rquote_nodes=1 rquote_insertion=1 npb_nodes=3 vp_nodes=2 period_nodes=1 pp_nodes=1 np_c_nodes=3 vbd_nodes=1 s_nodes=1 vp_c_nodes=1 vb_nodes=1 be_insertion=1 adjp_rooted=1 np_c_rooted=1 npb_rooted=1 s_rooted=1 s_c_rooted=1 s_c_nodes=1 nonmonotone=1 foreign-length=5 top-prob=0.147244 top-rule=1 nn_rooted=1 sb[JJ][CC]=1 sb[TO][VP-C]=1 sblm=38.7644 taglex.nrm.LDC2004E12=1.44 taglex.nrm.LDC2008G06=0.64 taglex.nrm.LDC2006E85=1.36 taglex.nrm.LDC2007E08=0.47 taglex.nrm.ng=1.71 taglex.nrm.LDC2003E07=0.6 taglex.nrm.LDC2005E83=1.62 taglex.nrm.LDC2007E103=0.38 taglex.nrm.CUDongaOct06=0.43 taglex.nrm.CUDongaOct07=0.32 taglex.nrm.un=1.5 taglex.nrm.LDC2006E34=-0.0100001 taglex.nrm.LDC2005T10=0.64 taglex.nrm.LDC2005T06=-0.44 taglex.nrm.LDC2006G05=1.62 taglex.nrm.wl=0.54 taglex.nrm.treebank=0.6 taglex.inv.wl=1.22 taglex.inv.LDC2002E18=1.19 taglex.inv.un=2.61 taglex.inv.LDC2006E86=0.88 taglex.inv.LDC2006E85=0.62 taglex.inv.LDC2004E12=2.37 taglex.inv.LDC2005T34=0.81 taglex.inv.bc=-1.06 taglex.inv.bn=-0.13 taglex.inv.CUDongaOct07=1.17 taglex.inv.LDC2005T06=-0.46 taglex.inv.LDC2006G05=2.07 taglex.inv.LDC2004T08_HK_News=1.48 taglex.inv.LDC2003E07=0.9 taglex.inv.LDC2006E24=-0.36 taglex.inv.LDC2006E26=1.61 taglex.inv.treebank=0.9 taglex.inv.LDC2008G06=1.63 taglex.inv.LDC2008G05=0.85 taglex.inv.LDC2007E101=-0.42 taglex.inv.ng=0.58 taglex.inv.LDC2005E83=1.04 taglex.inv.LDC2006E92=0.57 taglex.inv=5.82 taglex.inv.lexicon=0.7 taglex.inv.LDC2005T10=-0.51 taglex.inv.LDC2008E40=2.08 taglex.inv.LDC2007E06=1.14 taglex.inv.LDC2006E34=0.64 taglex.nrm.LDC2008G05=0.36 taglex.nrm.LDC2007E06=2.52 taglex.nrm.web=1.31 taglex.nrm.LDC2004T08_HK_News=0.25 taglex.nrm.LDC2007E46=1.79 taglex.nrm.LDC2006E93=0.7 taglex.nrm.LDC2006E24=1.18 taglex.nrm.bc=1.47 taglex.nrm.LDC2007E101=1.64 taglex.inv.Wikipedia=1.12 taglex.inv.government=0.51 taglex.inv.LDC2007E46=0.6 taglex.inv.LDC2007E87=-0.47 taglex.inv.LDC2006E93=0.63 taglex.inv.LDC2004T08=0.51 taglex.inv.web=1 taglex.inv.LDC2007E08=0.76 period_rooted=1 taglex.nrm.nw=0.52 taglex.nrm.LDC2006E92=2.48 taglex.nrm.bn=0.78 taglex.inv.LDC2007E103=0.45 taglex.nrm.LDC2007E87=1.55 taglex.inv.CUDongaOct06=0.62 taglex.inv.ADSO_v5=1.32 taglex.nrm.LDC2002L27=0.35 taglex.nrm.lexicon=0.31 taglex.nrm.LDC2008E40=0.62 count_category_4=2 taglex.nrm.LDC2002E18=-0.38 sblm-nts=30 sb[^(s^)][DT]=3 sb[DT][NN]=3 sb[NN][^(/s^)]=3 spc[NPB][DT]=3 spc[NPB][NN]=3 sb[^(s^)][NPB]=3 sb[NPB][PP]=1 sb[PP][^(/s^)]=1 spc[NP-C][NPB]=3 spc[NP-C][PP]=1 sb[^(s^)][IN]=1 sb[IN][NP-C]=1 sb[NP-C][^(/s^)]=1 spc[PP][IN]=1 spc[PP][NP-C]=1 sb[NPB][^(/s^)]=2 sb[^(s^)][JJ]=1 sb[CC][JJ]=1 sb[JJ][^(/s^)]=1 spc[ADJP][JJ]=2 spc[ADJP][CC]=1 sb[^(s^)][NP-C]=1 sb[NP-C][VP]=2 sb[VP][^(/s^)]=1 spc[S-C][NP-C]=1 spc[S-C][VP]=1 sb[^(s^)][TO]=1 sb[VP-C][^(/s^)]=1 spc[VP][TO]=1 spc[VP][VP-C]=1 sb[^(s^)][VB]=1 sb[VB][ADJP]=1 sb[ADJP][^(/s^)]=1 spc[VP-C][VB]=1 spc[VP-C][ADJP]=1 sb[^(s^)][S-C]=1 sb[S-C][^/]=1 sb[^/]['']=1 sb[''][NP-C]=1 sb[VP][.]=1 sb[.][^(/s^)]=1 spc[S][S-C]=1 spc[S][^/]=1 spc[S]['']=1 spc[S][NP-C]=1 spc[S][VP]=1 spc[S][.]=1 sb[^(s^)][VBD]=1 sb[VBD][^(/s^)]=1 spc[VP][VBD]=1 sb[^(s^)][S]=1 sb[S][^(/s^)]=1 spc[TOP][S]=1 headmarker={{{R(H(D(D(H(DH)D(HD(H(DH))))H(HD(HD(HDD))))DDD(H(DH))H(H)D))}}}
    '''
    f=re.compile(numfieldres)
    sb=inds(ns,'sb')
    spc=inds(ns,'spc')
    print dict_diff(sb,spc)

