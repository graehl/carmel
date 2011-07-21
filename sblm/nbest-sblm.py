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


def check_nbest(l,lm,term=True,strip=True,num2at=True):
    trees=getfield_brace('tree',l)
    t=str_to_tree(trees)
    rt=str(t)
    if trees!=rt:
        s=smallest_difference([trees,rt],porch=2)
        warn('tree={{{\n%s}}} mismatch with str(tree(t))=\n%s\nfull = %s'%(s[0],s[1],trees))
    def label(x):
        y=sbmt_lhs_label(x,num2at)
        return strip_subcat(y) if strip else y
    tm=t.mapnode(label)
    sb=inds(l,'sb')
    pc=inds(l,'spc')
    sbt=IntDict()
    pct=IntDict()
    def vpc(p,c):
#        dump(p,c)
        pct[(p,c)]+=1
    def vlr(left,right):
#        dump(left,right)
        sbt[(left,no_none(right,'</s>'))]+=1
    dump(str(tm))
    tm.visit_pcl(vpc,leaf=False,root=False)
    tm.visit_lrl(vlr,leaf=False,left='<s>',right='</s>')
    warn_diff(sb,sbt,desc='sb')
    warn_diff(pc,pct,desc='pc')
    #    log(str(tm))

@optfunc.arghelp('lm','SRI ngram trained by pcfg.py')
@optfunc.arghelp('nbest','sbmt_decoder nbest list (optionally with sblm score debugging logs before each nbest)')
def nbest_sblm_main(lm='nbest.pcfg.srilm',
                    nbest='nbest.txt',
                    strip=True,
                    num2at=True,
                    term=True,
                    ):
    n=None
#    log(escape_indicator('<s>_;_:_[]^'))
#    n=ngram(lm=lm)
    #outlm='rewrite.'+lm
    #n.write_lm(outlm)
    for l in open(nbest):
        if l.startswith("NBEST sent="):
            check_nbest(l,n,term,strip,num2at)

import optfunc
optfunc.main(nbest_sblm_main)
