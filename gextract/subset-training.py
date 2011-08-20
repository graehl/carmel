#!/usr/bin/env pypy
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
from etree import *

#warn(' '.join(sys.argv))

def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

INF=sys.maxint

import random

@optfunc.arghelp('upper_length','exclude lines whose # of english words is not in [lower,upper]')
@optfunc.arghelp('inbase','read inbase.{e-parse,a,f}')
@optfunc.arghelp('pcorrupt','if > 0, corrupt each link in output.a with this probability; write uncorrupted alignment to .a-gold')
@optfunc.arghelp('dcorrupt','move both the e and f ends of a distorted link within +-d')
@optfunc.arghelp('skip_includes_identity','skip sentences that do not have recall=1 for the identity alignment (#e = #f and (i,i) link is set for 0<=i<#f')
@optfunc.arghelp('begin','on line numbers pre-filtering, output lines from [begin,end)')
def subset_training(n_output_lines=INF,inbase="training",outbase="-",upper_length=INF,lower_length=0,begin=0,end=INF,monotone=False,pcorrupt=0.,dcorrupt=4,comment="",skip_identity=False,skip_includes_identity=False,align_in="",info_in="",etree_in="",estring_out=False,fileline=False,clean_eparse_out=False):
    "filter inbase.{e-parse,a,f} to outbase"
#    dump(str(Locals()))
    sys.stderr.write('### %s\n'%Locals())
    oa=open_out_prefix(outbase,".a")
    ina=open(align_in if align_in else inbase+".a")
    of=open_out_prefix(outbase,".f")
    inf=open(inbase+".f")
    oe=open_out_prefix(outbase,".e-parse")
    if estring_out:
        oes=open_out_prefix(outbase,".e")
    if clean_eparse_out:
        oeclean=open_out_prefix(outbase,".clean-e-parse")
    oinfo=open_out_prefix(outbase,".info")
    ine=open_first(etree_in,inbase+".e-parse")
    iinfo=open_first(info_in,inbase+".info")
    distort=pcorrupt>0
    if distort:
        oagold=open_out_prefix(outbase,".a-gold")
    n=0
    descbase=""
    if n_output_lines<INF:
        descbase+="first %d "%n_output_lines
    if upper_length<INF:
        descbase+="len<=%d "%upper_length
    if skip_identity:
        descbase+="non-identity "
    if monotone:
        descbase+="monotone "
    if distort:
        descbase+="(noise prob=%g +-%d) "%(pcorrupt,dcorrupt)
    for eline,aline,fline,lineno in itertools.izip(ine,ina,inf,itertools.count(0)):
        desc=descbase
        if comment:
            desc+="%s: "%comment
        fldesc="%s line %d"%(inbase,lineno)
        if iinfo is None:
            desc+=fldesc
        else:
            desc+=iinfo.readline().strip()+(" (%s)"%fldesc if fileline else "")

        if not (lineno>=begin and lineno<end): continue
        nf=len(fline.split())
        etree=raduparse(eline)
        estring=[]
        if etree is not None:
            estring=etree.yield_labels()
            ne=len(estring)
        else:
            ne=nf
        if ne>upper_length or ne<lower_length: continue
        if monotone:
            fline=' '.join([s.upper() for s in estring])+'\n'
            aline=' '.join(['%d-%d'%(i,i) for i in range(0,ne)])+'\n'
        a=Alignment(aline,ne,nf)
        if skip_identity and a.is_identity(): continue
        if skip_includes_identity and a.includes_identity(): continue
        if n>=n_output_lines: break
        n+=1

        if distort:
            oagold.write(aline)
            a.corrupt(pcorrupt,dcorrupt)
            aline=str(a)+'\n'
        oinfo.write(desc+"\n")
        if estring_out:
            oes.write(' '.join(estring)+'\n')
        if clean_eparse_out:
            oeclean.write(str(etree)+'\n')
        of.write(fline)
        oe.write(eline)
        oa.write(aline)
    log("%d lines written"%n)
optfunc.main(subset_training)

if False and __name__ == '__main__':
    o,_=usage.parse_args()
    subset_training(o.inbase,o.outbase,o.upper_length,o.lower_length,o.end,o.begin,o.monotone,o.n_output_lines)
