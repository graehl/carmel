#!/usr/bin/env pypy
from graehl import *
import sys
if __name__ == '__main__':
    for x in sys.argv[1:]:
        l,u=filedaterange(x,False)
        sys.stderr.write('%s %s\n'%(l,u))
        a=u-l if (u is not None and l is not None) else '???'
        l,u=(min(nonone((ctime(x),l))),max(nonone((mtime(x),u))))
        sys.stderr.write('%s %s\n'%(l,u))
        b=u-l
        print '%s\n%s\n\n'%(a,b),
