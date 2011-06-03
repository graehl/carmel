#!/usr/bin/env python2.6
"""
compute sblm score on nbest output

verify stdout from
 xrs_info_decoder --nbest-output - --sbmt.sblm.level=verbose --sbmt.sblm.file - --sbmt.ngram.file -

or just compare (or add) score if no component_score logs are kept
"""

import re
import tree
from graehl import *
from dumpx import *
from ngram import *
from pcfg import *


@optfunc.arghelp('terminals','include PRETERM("terminal") events')
def nbest_sblm_main(lm='nbest.pcfg.srilm',
                    nbest='nbest.txt',
                    ):
    n=ngram(lm=lm)
    dump(n.str())
    outlm='rewrite.'+lm
    n.write_lm(outlm)
import optfunc
optfunc.main(nbest_sblm_main)
