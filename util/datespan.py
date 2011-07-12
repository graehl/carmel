#!/usr/bin/env python2.6
from graehl import *
import sys

for x in sys.argv[1:]:
    l,u=filedaterange(x,False)
    sys.stderr.write('%s %s\n'%(l,u))
    a=u-l
    l,u=(min(ctime(x),l),max(mtime(x),u))
    sys.stderr.write('%s %s\n'%(l,u))
    b=u-l
    print '%s\n%s\n\n'%(a,b),
