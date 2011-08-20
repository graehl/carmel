#!/usr/bin/env python
import sys, itertools

# count.py
# input:  key \t ... \t count
# output: key \t sum

def stdinfields():
    for line in sys.stdin:
        yield line.rstrip().split("\t")

if __name__ == "__main__":
    for (key,records) in itertools.groupby(stdinfields(), lambda r: r[0]):
        sumcount = sum(int(r[-1]) for r in records)
        print "%s\t%s" % (key, sumcount)

