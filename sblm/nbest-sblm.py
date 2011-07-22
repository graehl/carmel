#!/usr/bin/env python2.7
"""
compute sblm score on nbest output

verify stdout from
 xrs_info_decoder --nbest-output - --sbmt.sblm.level=verbose --sbmt.sblm.file - --sbmt.ngram.file -

or just compare (or add) score if no component_score logs are kept
"""

import sys
import re
import tree
from graehl import *
from dumpx import *
from ngram import *
from pcfg import *
from tree import *
from nbest import *

sbpre='sb'
pcpre='spc'

unks=set([])
def tounk(x):
    return '<unk>' if x.startswith('GLUE') or x in unks else x

def check_nbest(l,lm,term=True,strip=True,flatten=True,num2at=True,output_nbest=None,maxnodes=999999,lineno='?'):
    trees=getfield_brace('tree',l)
    t=str_to_tree(trees)
    if t is None:
        warn("no tree for %s line #%s"%(trees,lineno))
        return False
    if t.size()>maxnodes:
        return False
    rt=str(t)
    if trees!=rt:
        s=' '.join(smallest_difference([trees,rt],porch=2))
        warn('mismatch %s \nfull = %s'%(s,trees))
    def label(x):
        return tounk(sbmt_lhs_label(x,num2at))
    tm=t.mapnode(label)
    def skiplabel(y):
        if strip:
            y=strip_subcat(y)
        return no_bar(y) if flatten else y
    tm=tm.map_skipping(skiplabel)
    fv=getfields_num(l)
    sb=inds(fv,sbpre)
    pc=inds(fv,pcpre)
    fvd=dict(fv)
    sent=fvd['sent']
    sbt=IntDict()
    pct=IntDict()
    def vpc(p,c):
        pct[(p,c)]+=1
    def vlr(left,right):
        sbt[(left,no_none(right,'</s>'))]+=1
    tm.visit_pcl(vpc,leaf=False,root=False)
    tm.visit_lrl(vlr,leaf=False,left='<s>',right='</s>')
    head='sent=%s tree=%s tree-orig=%s'%(sent,tm,trees)
    if len(sb):
        warn_diff(sb,sbt,desc=sbpre,header=head)
    if len(pc):
        warn_diff(pc,pct,desc=pcpre,header=head)
    if output_nbest is not None:
        s=stripinds(l.rstrip(),'%s|%s'%(sbpre,pcpre))
        dump(s)
        output_nbest.write('%s %s %s\n'%(s,strinds(sbpre,sbt),strinds(pcpre,pct)))
    return True

@optfunc.arghelp('lm','SRI ngram trained by pcfg.py')
@optfunc.arghelp('nbest','sbmt_decoder nbest list (optionally with sblm score debugging logs before each nbest)')
def nbest_sblm_main(lm='nbest.pcfg.srilm',
                    nbest='nbest.txt',
                    strip=True,
                    flatten=True,
                    num2at=True,
                    term=True,
                    output_nbest='',
                    maxnodes=999999,
                    ):
    n=None
#    log(escape_indicator('<s>_;_:_[]^'))
#    n=ngram(lm=lm)
    #outlm='rewrite.'+lm
    #n.write_lm(outlm)
    dump(nbest)
    output_nbest=None if output_nbest=='' else open(output_nbest,'w')
    n=0
    ng=0
    for l in open(nbest):
        if l.startswith("NBEST sent="):
            n+=1
            if check_nbest(l,n,term,strip,flatten,num2at,output_nbest,maxnodes,lineno=n):
                ng+=1
    log("%s good out of %s NBEST lines"%(ng,n))

import optfunc
optfunc.main(nbest_sblm_main)
