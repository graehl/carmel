#!/usr/bin/env python3

usage = '''
Show e,f,a (tab separated) in a more legible format with alignment links after word {#i:j k}. e and f are on alternating lines of output
alignment a is pairs (s t)* where s 0-based indexes e, t 0-based indexes f. indices > #words (space sep) in e or f are considered NULL alignments (not aligned to any word) and ignored.
'''

import argparse
import sys

parser=argparse.ArgumentParser(description=usage)

def aword(i, a, w):
    return '{%d:%s}%s' % (i, ' '.join(map(str, a)), w)

def awords(a, w):
    return ' '.join(aword(i, a[i], w[i]) for i in range(len(w)))


def forfiles(infiles):
    for f in infiles:
        for line in f:
            forline(line)

def forline(line):
    fields = line.split('\t')
    if len(fields) < 3: return
    S, T, A = fields[-3:]
    S = S.split()
    T = T.split()
    A = A.split()
    al = [(int(A[i]), int(A[i+1])) for i in range(0, len(A), 2)]
    s2t = [[] for _ in S]
    t2s = [[] for _ in T]
    for s,t in al:
        if s < len(S) and t < len(T):
            s2t[s].append(t)
            t2s[t].append(s)
    print(awords(s2t, S))
    print(awords(t2s, T))
    print

def main(infiles):
    forfiles([open(x, 'r') for x in infiles] if len(infiles) else [sys.stdin])

if __name__ == '__main__':
    main(sys.argv[1:])
