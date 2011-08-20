#!/usr/bin/env pypy
# Dyer's etrees have bare SYM or NNP etc. when it should be (SYM @) or (NNP @)

import os,sys
sys.path.append(os.path.dirname(sys.argv[0]))

import optfunc, itertools
from graehl import *
from dumpx import *

default_in="-"
default_in="5.ar-en.al"

import tree
def raduparse(t,radu=True):
    if radu: t=radu2ptb(t)
    return tree.str_to_tree(t)

INF=sys.maxint

def fix_yield_word(w):
    if w=='``' or w=="''":
        return '"'
    return w

def check_parse(inbase='',a='',e='',parse='',f='',outbase='',yield_out='',tree_out='',radu_out=True,radu_in=True,lowercase=True,dyer_sym=True,max_in=INF,skip_mismatch=True,rewrite_yield=True):
    if inbase:
        if not e: e="%s.e"%inbase
        if not parse: parse="%s.e-parse"%inbase
        if not f: f="%s.f"%inbase
        if not a: a="%s.a"%inbase
    of=oe=op=oa=oi=oyield=None
    if outbase:
        of,oe,op,oa,oi=map(lambda x:open_out_prefix(outbase,"."+x),['f','e','e-parse','a','info'])
    if yield_out:
        oyield=open_out(yield_out)
    if tree_out:
        op=open_out(tree_out)
    blanks=[]
    syms=IntDict()
    badlines=IntDict()
    mismatches=[]
    no=0
    for el,pl,fl,al in itertools.izip(open_default_line(e),open_in(parse),open_default_line(f),open_default_line(a)):
        if no>=max_in:
            warn("stopping early after %d input lines"%max_in)
            break
        no+=1
        info="%s line %d"%(parse,no)
        pes=etree=None
        if el: el=el.strip()
        try:
            etree=raduparse(pl,radu_in)
            if etree is None and el is None or el=="":
                blanks.append(no)
                continue
    #        dump(str(etree),etree.yield_labels())
            if dyer_sym:
                for t in etree.frontier():
                    p=t.parent
                    if len(p.children)!=1:
                        t.append_child(Node('@'))
                        syms[no]+=1
                        warn("attached missing terminal '@' to %s in context %s"%(t,p))
            pes=etree.yield_labels()
            if rewrite_yield:
                pes=map(fix_yield_word,pes)
        except:
            bad=(info,etree,el)
            badlines[el]+=1
            warn("bad etree for %s:\netree: %s\nestring: %s"%bad)
            continue
        if el is not None:
            es=el.strip().split()
            if lowercase:
                es=[x.lower() for x in es]
                pes=[x.lower() for x in pes]
            if len(es)!=len(pes):
                warn("line %d .e-parse has %d leaves but .e has %d words"%(no,len(pes),len(es)))
            if es!=pes:
                fstr=" .f={{{%s}}}"%fl.strip() if fl else ''
                warn("line %d %s .e={{{%s}}} .e-parse={{{%s}}}%s"%(no,mismatch_text(pes,es,"e-parse-yield","e"),' '.join(es),etree,fstr))
                mismatches.append(no)
                if skip_mismatch:
                    continue
        if oyield: oyield.write(' '.join(pes)+'\n')
        if oe: oe.write(el+'\n')
        if op: op.write(etree.str(radu_out)+'\n')
        if oa: oa.write(al)
        if oi: oi.write(info+'\n')
        if of: of.write(fl)
    if len(blanks):
        warn("%d blank lines: %s"%(len(blanks),blanks))
    if len(badlines):
        warn("%d bad lines: %s"%(sum(badlines.itervalues()),badlines))
    if len(syms):
        warn("%d missing '@' terminals restored: %s"%(sum(syms.itervalues()),syms))
    if len(mismatches):
        warn("%d .e strings mismatch: %s"%(len(mismatches),mismatches))
    sys.stderr.write("%d parses OK.\n"%no)

optfunc.main(check_parse)
