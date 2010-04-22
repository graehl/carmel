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

@optfunc.arghelp('upper_length','exclude lines whose # of english words is not in [lower,upper]')
@optfunc.arghelp('inbase','read inbase.{e-parse,a,f}')
def subset_training(inbase="astronauts",outbase="-",upper_length=sys.maxint,lower_length=0,end=sys.maxint,begin=0,monotone=True,n_output_lines=sys.maxint):
    "filter inbase.{e-parse,a,f} to outbase"
    oa=open_out_prefix(outbase,".a")
    ina=open(inbase+".a")
    of=open_out_prefix(outbase,".f")
    inf=open(inbase+".f")
    oe=open_out_prefix(outbase,".e-parse")
    ine=open(inbase+".e-parse")

    n=0
    for eline,aline,fline,lineno in itertools.izip(ine,ina,inf,itertools.count(0)):
        if not (lineno>=begin and lineno<end): continue
        etree=raduparse(eline)
        estring=etree.yield_labels()
        ne=len(estring)
        if ne>upper_length or ne<lower_length: continue
        if n>=n_output_lines: break
        n+=1
        if monotone:
            fline=' '.join([s.upper() for s in estring])+'\n'
            aline=' '.join(['%d-%d'%(i,i) for i in range(0,ne)])+'\n'
        oa.write(aline)
        of.write(fline)
        oe.write(eline)

def optparse_main():
    o,_=usage.parse_args()
    subset_training(o.inbase,o.outbase,o.upper_length,o.lower_length,o.end,o.begin,o.monotone,o.n_output_lines)

if __name__ == '__main__':
    optfunc.run(subset_training)
    #    optfunc.header='commands:\n'
    #    optfunc.run([subset_training])
    #    optparse_main()
