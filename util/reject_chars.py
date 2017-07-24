#! /usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function
#from __future__ import unicode_literals
# hack for python2/3 compatibility
import argparse, sys
reload(sys)
sys.setdefaultencoding('utf-8')
import codecs
from io import open
def openutf8(f):
    return codecs.open(f.name, encoding='utf-8')

argparse.open = open
# python 2/3 compatibility
if sys.version_info < (3, 0):
    sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
    sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
    sys.stdin = codecs.getreader('UTF-8')(sys.stdin)
else:
    sys.stderr = codecs.getwriter('UTF-8')(sys.stderr.buffer)
    sys.stdout = codecs.getwriter('UTF-8')(sys.stdout.buffer)
    sys.stdin = codecs.getreader('UTF-8')(sys.stdin.buffer)

from collections import Counter

verbose=0

def log(s, out=sys.stderr):
    out.write("### %s\n" % (s,))

def logv(v, s, out=sys.stderr):
    if verbose >= v: log(s, out)

def read_chars(a):
    counts = Counter()
    totalcount = 0.0
    for line in a.chars_vocab:
        if len(line) > 2:
            if line[1] == ' ':
                char = line[0]
                countplus = line[2:].split()
                count = int(countplus[0])
                counts[char] += count
                totalcount += count
    need = a.min_freq
    log(need)
    need2 = totalcount * a.min_prob
    log(need2)
    if need < need2: need = need2
    topk = a.atleast_topk
    if topk > 0:
        topcounts = sorted(counts.values(), reverse=True)
        topkneed = topcounts[topk] if topk < len(topcounts) else topcounts[-1] if len(topcounts) else 0
        log('topkneed: %s'%topkneed)
        if need > topkneed: need = topkneed
        log('need: %s'%need)
    keep = dict()
    for k, c in counts.items():
        if c >= need:
            keep[k] = c
    log('keep %s: %s'%(len(keep), keep))

    return keep

def nrare(line, common):
    n = 0
    for c in line:
        if c not in common:
            n += 1
    return n

def keep_line(line, a, c):
    l = a.line_allow_rare
    k = a.addk_allow_rare
    r = nrare(line, c)
    return float(r) / (k + len(line)) <= l

def create_parser():
    p = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="reject lines with too many low-count characters (using chars_vocab.py output)")
    p.add_argument('--out-drop-lineno', '-y', type=argparse.FileType('w'), default=None, help="write sorted line nos of dropped lines here")
    p.add_argument('--drop-lineno', '-Y', type=argparse.FileType('r'), default=None, help="read line nos to be dropped from here (in addition to current criteria)")
    p.add_argument('--chars-vocab', '-c', type=argparse.FileType('r'), metavar='PATH', help="lines of char count")
    p.add_argument('--verbose', '-v', type=int, default=1, help='0 => no default stderr output')
    p.add_argument('--rejected-lines', '-j', type=argparse.FileType('w'), default=None, metavar='PATH', help="write rejected lineno TAB line here")
    p.add_argument('--replaced-rejected-lines', '-J', type=argparse.FileType('w'), default=None, metavar='PATH', help="write rejected replaced(line) here")
    p.add_argument('--input', '-i', type=argparse.FileType('r'), default=sys.stdin, help="input file")
    p.add_argument('--output', '-o', type=argparse.FileType('w'), default=sys.stdout, help="output file")
    p.add_argument('--min-freq', '-m', type=int, default=1, help="minimum count of char to keep (or min-prob whichever is higher)")
    p.add_argument('--min-prob', '-p', type=float, default=.0001, help="minimum unigram probability of char to keep (or min-freq whichever is higher)")
    p.add_argument('--no-replace', '-R', action="store_true", help="neither delete nor replace low freq chars (but do filter lines)")
    p.add_argument('--replacement-char', '-r', type=str, default=unichr(0xfdea).encode('utf-8'), help="replace low-count chars with this - default = U+FDEA, a private noncharacter (use -d if you don't want them in output at all)")
    p.add_argument('--runs', '-u', action="store_true", help="when >1 replacement-char in a row, output more than one. by default they're collapsed to a single output")
    p.add_argument('--delete', '-d', action="store_true", help="don't use replacement-char - remove rare char")
    p.add_argument('--atleast-topk', '-a', type=int, default=0, help="keep at least this many char types even if below min-freq or min-prob")
    p.add_argument('--line-allow-rare', '-l', type=float, default=0.2, help="allow at most this portion of chars to be rare")
    p.add_argument('--addk-allow-rare', '-k', type=float, default=1, help="add this to denom of rare-chars fraction")
    return p

def replaced(line, a, common):
    if a.no_replace:
        return line
    r = a.replacement_char
    o = ''
    run = False
    for c in line:
        if c not in common:
            if not a.delete:
                if a.runs or not run:
                    o += r
            run = True
        else:
            run = False
            o += c
    return o

if __name__ == '__main__':

    p = create_parser()
    a = p.parse_args()
    c = read_chars(a)
    verbose = a.verbose
    if verbose >= 1:
        if a.rejected_lines is None:
            a.rejected_lines = sys.stderr
    lineno = 0
    drops = set()
    if a.drop_lineno is not None:
        for line in a.drop_lineno:
            fields = line.split()
            drops.add(int(fields[0]))
    log('#drops=%s drops[0:10]=%s'%(len(drops), sorted(drops)[0:10]))
    for line in a.input:
        lineno += 1
        line = line.rstrip("\r\n")
        if lineno not in drops and keep_line(line, a, c):
            print(replaced(line, a, c), file=a.output)
        else:
            if a.out_drop_lineno:
                print(u'%s'%lineno, file=a.out_drop_lineno)
            if a.rejected_lines is not None:
                print(u"%s\t%s"%(lineno, line), file=a.rejected_lines)
            if a.replaced_rejected_lines is not None:
                print(replaced(line, a, c), file=a.replaced_rejected_lines)
