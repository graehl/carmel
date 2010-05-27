#!/usr/bin/env python2.6
version=".9"
doc="""create reduced .a .e-parse .f limited to shorter lines, from line M to N, optionally f cloned from e string with (nearly) monotone alignment
"""

import optfunc

import optparse,sys,tree,itertools

usage=optparse.OptionParser(epilog=doc,version="%prog "+version)
usage.add_option("-i","--inbase",dest="inbase",metavar="PREFIX",help="input lines from PREFIX.{e-parse,a,f}")
usage.add_option("-o","--outbase",dest="outbase",metavar="PREFIX",help="output PREFIX.{e-parse,a,f} instead of stdout")
usage.add_option("-u","--upper-length",dest="upperl",metavar="LIMIT_SUP",help="include only sentences whose english string is no longer than LIMIT_SUP",type="int")
usage.add_option("-l","--lower-length",dest="lowerl",metavar="LIMIT_INF",help="include only english strings at least LIMIT_INF long",type="int")
usage.add_option("-b","--begin",dest="begin",help="skip the first N sentences",type="int")
usage.add_option("-e","--end",dest="end",help="stop at sentence N (end-begin)=max number of lines output",type="int")
usage.add_option("-m","--monotone",dest="monotone",action="store_true",help="ignore .a and .f, producing a monotone aligned f=e")
usage.add_option("--output-lines",dest="n",type="int",help="stop after n lines of output")
usage.set_defaults(inbase="astronauts",outbase="-",monotone=False,begin=0,end=sys.maxint,lowerl=0,upperl=sys.maxint,n=sys.maxint
#                   ,outbase="-",begin=1,end=2,monotone=True
                   )


from graehl import *
from dumpx import *

import tree
def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

INF=sys.maxint

import random

@optfunc.arghelp('upper_length','exclude lines whose # of english words is not in [lower,upper]')
@optfunc.arghelp('inbase','read inbase.{e-parse,a,f}')
@optfunc.arghelp('pcorrupt','if > 0, corrupt each link in output.a with this probability; write uncorrupted alignment to .a-gold')
@optfunc.arghelp('dcorrupt','move both the e and f ends of a distorted link within +-d')
def subset_training(inbase="training",outbase="-",upper_length=INF,lower_length=0,begin=0,end=INF,monotone=False,n_output_lines=INF,pcorrupt=0.,dcorrupt=4,comment="",skip_identity=False,align_in=""):
    "filter inbase.{e-parse,a,f} to outbase"
    dump(str(Locals()))
    oa=open_out_prefix(outbase,".a")
    ina=open(align_in if align_in else inbase+".a")
    of=open_out_prefix(outbase,".f")
    inf=open(inbase+".f")
    oe=open_out_prefix(outbase,".e-parse")
    oinfo=open_out_prefix(outbase,".info")
    ine=open(inbase+".e-parse")
    try:
        iinfo=open(inbase+".info")
    except:
        iinfo=None
    distort=pcorrupt>0
    if distort:
        oagold=open_out_prefix(outbase,".a-gold")
    n=0
    descbase=""
    if upper_length<INF:
        descbase+="len<=%d "%upper_length
    if skip_identity:
        descbase+="non-identity "
    if n_output_lines<INF:
        descbase+="(head -%d) "%n_output_lines
    if monotone:
        descbase+="monotone "
    if distort:
        descbase+="(w/ prob=%g alignments +-%d) "%(dcorrupt,pcorrupt)
    for eline,aline,fline,lineno in itertools.izip(ine,ina,inf,itertools.count(0)):
        desc=descbase
        if comment:
            desc+="%d: "%comment
        if iinfo is None:
            desc+="%s.{e-parse,a,f} line %d"%(inbase,lineno)
        else:
            desc+=iinfo.readline().strip()
        if not (lineno>=begin and lineno<end): continue
        etree=raduparse(eline)
        estring=[]
        if etree is not None:
            estring=etree.yield_labels()
        ne=len(estring)
        if ne>upper_length or ne<lower_length: continue
        if n>=n_output_lines: break
        n+=1
        if monotone:
            fline=' '.join([s.upper() for s in estring])+'\n'
            aline=' '.join(['%d-%d'%(i,i) for i in range(0,ne)])+'\n'
        nf=len(fline.split())
        a=Alignment(aline,ne,nf)
        if skip_identity and a.is_identity(): continue
        if distort:
            oagold.write(aline)
            a.corrupt(pcorrupt,dcorrupt)
            aline=str(a)+'\n'

        oa.write(aline)
        of.write(fline)
        oe.write(eline)
        oinfo.write(desc+"\n")
optfunc.main(subset_training)

if False and __name__ == '__main__':
    o,_=usage.parse_args()
    subset_training(o.inbase,o.outbase,o.upper_length,o.lower_length,o.end,o.begin,o.monotone,o.n_output_lines)
