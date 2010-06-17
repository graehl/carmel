#!/usr/bin/env python2.6

import os,sys
sys.path.append(os.path.dirname(sys.argv[0]))

import optfunc, itertools
from graehl import *
from dumpx import *

default_in="-"
default_in="5.ar-en.al"

import tree
def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

def check_parse(input='',e='',parse='',f='',yield_out='',tree_out='',radu_tree_format=True):
    if input:
        if not e: e="%s.e"%input
        if not parse: parse="%s.e-parse"%input
        if not f: f="%s.f"%input
    if yield_out:
        of=open_out(yield_out)
    if tree_out:
        ot=open_out(tree_out)
    for el,pl,fl,no in itertools.izip(open_in(e),open_in(parse),open_default_line(f),itertools.count(1)):
        es=el.strip().split()
        etree=raduparse(pl)
#        dump(str(etree),etree.yield_labels())
        pes=etree.yield_labels()
        if yield_out:
            of.write(' '.join(pes)+'\n')
        if tree_out:
            ot.write(etree.str(radu_tree_format)+'\n')
        if len(es)!=len(pes):
            warn("line %d .e-parse has %d leaves but .e has %d words"%(no,len(pes),len(es)))
        if es!=pes:
            fstr=" .f={{{%s}}}"%fl.strip() if fl else ''
            warn("line %d %s .e={{{%s}}} .e-parse={{{%s}}}%s"%(no,mismatch_text(pes,es,"e-parse-yield","e"),' '.join(es),etree,fstr))

optfunc.main(check_parse)
