#!/usr/bin/env python
import sys, itertools

# count.py
# input:  key \t ... \t count
# output: key \t sum

def input():
    for line in sys.stdin:
        yield line.rstrip().split("\t")

for key, records in itertools.groupby(input(), lambda record: record[0]):
    sumcount = sum(int(record[-1]) for record in records)
    print "%s\t%s" % (key, sumcount)
        
