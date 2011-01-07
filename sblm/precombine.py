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
    buffer_size = int(opts.get('-b', 100000))

    buffer = collections.defaultdict(list)
    count = 0
    for line in sys.stdin:
        record = line.rstrip().split('\t')
        key = tuple(record[:n_keys])
        buffer[key].append(record)
        count += 1

        if count >= buffer_size:
            for key, records in buffer.iteritems():
                for record in records:
                    print "\t".join(record)
            buffer.clear()
            count = 0

    for key, records in buffer.iteritems():
        for record in records:
            print "\t".join(record)        
