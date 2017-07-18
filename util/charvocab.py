#! /usr/bin/env python3
from __future__ import print_function
import sys
import codecs
from collections import Counter

# python 2/3 compatibility
if sys.version_info < (3, 0):
    sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
    sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
    sys.stdin = codecs.getreader('UTF-8')(sys.stdin)
else:
    sys.stderr = codecs.getwriter('UTF-8')(sys.stderr.buffer)
    sys.stdout = codecs.getwriter('UTF-8')(sys.stdout.buffer)
    sys.stdin = codecs.getreader('UTF-8')(sys.stdin.buffer)

c = Counter()

for line in sys.stdin:
    for char in line.rstrip("\r\n"):
        c[char] += 1

for key,f in sorted(c.items(), key=lambda x: x[1], reverse=True):
    print("%s %s # U+%04x"%(key, f, ord(key)))
