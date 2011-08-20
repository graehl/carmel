#!/usr/bin/env python
import sys, getopt, collections

# precombine.py [-k <keysize>] [-b <bufsize>]
# prepare map output for input to a combiner
# <keysize> = number of key fields (default 1)
# <bufsize> = maximum number of records in buffer (default 100000)

if __name__ == "__main__":
    opts, args = getopt.gnu_getopt(sys.argv[1:], 'k:b:')
    opts = dict(opts)

    n_keys = int(opts.get('-k', 1))
    buf_size = int(opts.get('-b', 100000))

    buf = collections.defaultdict(list)
    count = 0
    for line in sys.stdin:
        record = line.rstrip().split('\t')
        key = tuple(record[:n_keys])
        buf[key].append(record)
        count += 1

        if count >= buf_size:
            for key, records in buf.iteritems():
                for record in records:
                    print "\t".join(record)
            buf.clear()
            count = 0

    for key, records in buf.iteritems():
        for record in records:
            print "\t".join(record)
