#!/usr/bin/env python2.6
version=".9"
doc="""create reduced .a .e-parse .f limited to shorter lines, from line M to N, optionally f cloned from e string with (nearly) monotone alignment
"""

import optparse,sys,tree,itertools

usage=optparse.OptionParser(epilog=doc,version="%prog "+version)
usage.add_option("-r","--inbase",dest="inbase",metavar="PREFIX",help="input lines from PREFIX.{e-parse,a,f}")
usage.add_option("-w","--outbase",dest="outbase",metavar="PREFIX",help="output PREFIX.{e-parse,a,f} instead of stdout")
usage.add_option("-u","--upper-length",dest="upperl",metavar="LIMIT_SUP",help="include only sentences whose english string is no longer than LIMIT_SUP",type="int")
usage.add_option("-l","--lower-length",dest="lowerl",metavar="LIMIT_INF",help="include only english strings at least LIMIT_INF long",type="int")
usage.add_option("-b","--begin",dest="begin",help="skip the first N sentences",type="int")
usage.add_option("-e","--end",dest="end",help="stop at sentence N (end-begin)=max number of lines output",type="int")
usage.add_option("-m","--monotone",dest="monotone",action="store_true",help="ignore .a and .f, producing a monotone aligned f=e")
usage.add_option("-n","--output-lines",dest="n",type="int",help="stop after n lines of output")
usage.set_defaults(inbase="astronauts",outbase="-",monotone=False,begin=0,end=sys.maxint,lowerl=0,upperl=sys.maxint,n=sys.maxint
#                   ,outbase="-",begin=1,end=2,monotone=True
                   )
o,_=usage.parse_args()


from graehl import *
from dumpx import *

oa=open_out_prefix(o.outbase,".a")
ina=open(o.inbase+".a")
of=open_out_prefix(o.outbase,".f")
inf=open(o.inbase+".f")
oe=open_out_prefix(o.outbase,".e-parse")
ine=open(o.inbase+".e-parse")

import tree
def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

n=0
for eline,aline,fline,lineno in itertools.izip(ine,ina,inf,itertools.count(0)):
    if not (lineno>=o.begin and lineno<o.end): continue
    etree=raduparse(eline)
    estring=etree.yield_labels()
    ne=len(estring)
    if ne>o.upperl or ne<o.lowerl: continue
    if n>=o.n: break
    n+=1
    if o.monotone:
        fline=' '.join([s.upper() for s in estring])+'\n'
        aline=' '.join(['%d-%d'%(i,i) for i in range(0,ne)])+'\n'
    oa.write(aline)
    of.write(fline)
    oe.write(eline)
