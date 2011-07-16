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

def escape_indicator(x):
    for (a,b) in [('^','^^'), ('[','^{'),(']','^}'),(',','^/'),(':','^;'),('<','^('),('>','^)')]:
        x=x.replace(a,b)
    return x

def check_nbest(l,lm,num2at=True):
    trees=getfield_brace('tree',l)
    t=str_to_tree(trees)
    rt=str(t)
    if trees!=rt:
        s=smallest_difference([trees,rt],porch=2)
        warn('tree={{{\n%s}}} mismatch with str(tree(t))=\n%s\nfull = %s'%(s[0],s[1],trees))
    def label(x):
        return sbmt_lhs_label(x,num2at)
    tm=t.mapnode(label)
    log(str(tm))
    pass

@optfunc.arghelp('lm','SRI ngram trained by pcfg.py')
@optfunc.arghelp('nbest','sbmt_decoder nbest list (optionally with sblm score debugging logs before each nbest)')
def nbest_sblm_main(lm='nbest.pcfg.srilm',
                    nbest='nbest.txt',
                    ):
    n=None
#    log(escape_indicator('<s>_;_:_[]^'))
#    n=ngram(lm=lm)
    #outlm='rewrite.'+lm
    #n.write_lm(outlm)
    for l in open(nbest):
        if l.startswith("NBEST sent="):
            check_nbest(l,n)

import optfunc
optfunc.main(nbest_sblm_main)
