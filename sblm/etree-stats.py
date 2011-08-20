#!/usr/bin/env pypy
"""

output NT, preterm vocab, (stripping -N cat-split suffixes, optionally), and optionally any restructured node->eventual treebank parent distributions.

"""

import re
import tree
from collections import defaultdict
from graehl import *
from dumpx import *
from ngram import *
from pcfg import *

small=True

dev='data/dev.e-parse'
test='data/test.e-parse'
train='data/train.e-parse'
inpre=dev if small else train
binsuf='.binarized.linkdel.with_head_info' if small else '.binarized.linkdel'
ntsuf='.vocab.nts'
ptsuf='.vocab.pts'
parents='.parents'


def strip_n(s):
    return s

def strip_bar(s):
    return s[:-4] if s.endswith('-BAR') else s

def yield_node_ancestors(t,node_pred,passed=None):
    "recurse on t, yielding visit(n,a_self,a_above) for each n in t, with a_self = n if node_pred(n), else a_above, where a_above is nearest ancestor with node_pred(a_above). a_above and a_self may be passed=None"
    if node_pred(t):
        yield (t,t,passed)
        passed=t
    else:
        yield (t,passed,passed)
    for c in t.children:
        for p in yield_node_ancestors(c,node_pred,passed):
            yield p

def etree_stats_main(inpre=inpre
                    ,outpre=''
                    ,bin=''
                    ,binsuf=binsuf
                    ,compare_bin=True
                    ,strip_catsplit=True
                     ,load_vocab=False
                     ,heads=True
                     ,head_words=True
                    ):
    log('etree-stats')
    log(str(Locals()))
    nt=set()
    pt=set()
    headwords=defaultdict(lambda:IntDict())
    headtags=defaultdict(lambda:IntDict())
    input=open(inpre)
    if not len(outpre):
        outpre=inpre
    dump(outpre)
    maplabel=strip_n if strip_catsplit else identity
    if load_vocab:
        nt=set(l.strip() for l in open(inpre+ntsuf))
    else:
        for line in input:
            # warn('tree line',line,max=1)
            t=raduparse(line,intern_labels=False,strip_head=False)
            # warn('tree parsed',t,max=1)
            if t is None:
                continue
            for n in t.preorder():
                if heads and hasattr(n,'head_children'):
                    hc=n.head_children
                    nc=len(n.children)
                    cat=n.label
                    hw=n.headword
                    if not n.good_head:
                        hw=('BADHEAD','badhead')
                    if head_words: headwords[cat][hw]+=1
                    headtags[cat][hw[0]]+=1
                if n.is_terminal():
                    pass
                else:
                    (pt if n.is_preterminal() else nt).add(maplabel(n.label))
        a=write_lines(sorted(nt),outpre+ntsuf)
        b=write_lines(sorted(pt),outpre+ptsuf)
        callv(['head','-10',a,b])
    if (compare_bin):
        if not len(bin):
            bin=inpre+binsuf
        binput=open(bin)

        ntpt=nt.union(pt)
        def istreebank(t):
            return t.label in ntpt
        ps=defaultdict(lambda: IntDict())

        for line in binput:
            t=raduparse(line,intern_labels=False)
            if t is None:
                continue
            for t,tself,_ in yield_node_ancestors(t,istreebank):
                if t.is_terminal():
                    continue
                tl,pl=t.label,tself.label
                if tl!=pl:
                    if ps[tl][pl]==0:
                        log('treebank ancestor differs - %s >= %s'%(tl,pl))
                        if pl!=strip_bar(tl):
                            warn('treebank ancestor differs and not just by dropping -BAR: ','%s >= %s in tree %s'%(tl,pl,tself),max=None)
                ps[tl][pl]+=1
        outp=open(outpre+parents,'w')
        #outp=sys.stdout
        #dump(ps)
        for t in sorted(ps.keys()):
            outp.write('%s under:\n'%t)
            pst=ps[t]
            if len(pst)>1:
                warn("tag type has more than 1 parent tag type: ",t,max=None)
            write_dict(pst,out=outp)
            outp.write('\n')

    if heads:
        tf,hf=map(lambda x:outpre+x,('.headtag','.headword'))
        #write_nested_counts(headtags)
        write_nested_counts(headtags,out=tf)
        if head_words:
            write_nested_counts(headwords,out=hf)
            callv(['head','-n','1',tf,hf])
    info_summary()

import optfunc
optfunc.main(etree_stats_main)
#if __name__ == "__main__":
#    pcfg_ngram_main()

"""
TODO:

check how cat-split NP-2 vs. -BAR interacts. implement ignore-cat option

"""
