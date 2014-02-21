#!/usr/bin/python

import sys
import md5
import itertools

def reproducible_random(seed):
    """ chain md5 """
    hash = seed
    while True:
        digest = md5.md5(hash).digest()
        for c in digest:
            yield ord(c)
        hash = digest + hash[0:len(digest)]

def usage():
    sys.stderr.write("arg1 = # of bytes, arg2 = seed\n")
    sys.exit(1)

def main(args):
    seed = "random-c-array.py-encrypted-seed-seed"
    n = 32
    if len(args) >= 1:
        n = int(args[0])
        if len(args) == 2:
            seed =args[1]
        elif len(args) > 2:
            usage()
    sys.stderr.write('[n=%s] [seed=%s]\n'%(n, seed))
    bytes = itertools.islice(reproducible_random(seed), n)
    print ', '.join(map(str, bytes))

if __name__ == '__main__':
    main(sys.argv[1:])
