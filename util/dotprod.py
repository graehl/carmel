#!/usr/bin/env python
import sys

def readmap(f):
    m = {}
    if type(f) == str:
        f = open(f, 'r')
    for line in f:
        (k, v) = line.split()
        if k in m:
            raise "duplicate %s" % k
        m[k] = float(v)
    return m


def dotprod(m1, m2):
    sum = 0.0
    for k in m1:
        if k in m2:
            v1 = m1[k]
            v2 = m2[k]
            sys.stderr.write('+ (%s * %s = %s) // %s\n' % (v1, v2, v1 * v2, k))
            sum += v1 * v2
    return sum

(f1, f2) = sys.argv[1:]
m1 = readmap(f1)
m2 = readmap(f2)
print dotprod(m1, m2)
