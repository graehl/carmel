#!/usr/bin/env python3

usage = '''
Show e,f,a (tab separated) in a more legible format with alignment links after word {#i j k}.
alignment a is pairs (s t)* where s 0-based indexes e, t 0-based indexes f. indices > #words (space sep) in e or f are considered NULL alignments (not aligned to any word) and ignored.
'''

import argparse
import sys

parser=argparse.ArgumentParser(description='translate (utf8) input file lines to C header files (without include guards or namespaces)')

def aword(i, a, w):
    return '{%d:%s}%s' % (i, ' '.join(map(str, a)), w)

def awords(a, w):
    return ' '.join(aword(i, a[i], w[i]) for i in range(len(w)))

for line in sys.stdin:
    fields = line.split('\t')
    if len(fields) < 3: continue
    S, T, A = fields[-3:]
    S = S.split()
    T = T.split()
    A = A.split()
    al = [(int(A[i]), int(A[i+1])) for i in range(0, len(A), 2)]
    s2t = [[] for _ in S]
    t2s = [[] for _ in T]
    print(s2t)
    for s,t in al:
        if s < len(S) and t < len(T):
            s2t[s].append(t)
            t2s[t].append(s)
    print(awords(s2t, S))
    print(awords(t2s, T))
