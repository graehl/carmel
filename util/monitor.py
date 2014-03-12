#!/home/nbuild/local/bin/python
# -*- coding: utf-8 -*-

# Markus Dreyer, mdreyer@sdl.com

from __future__ import print_function
from __future__ import division
# import glob
# import optparse
# import tempfile
from timeit import default_timer as timer
import logging
import os
import psutil
import subprocess
import sys
import time

# from lw.mdreyer.misc import mkdir_p

LOG = logging.getLogger(__name__)

def peakstr(peak):
    (t, rss, vms, lines) = peak
    return " @t=%s s, Peak-RSS=%s, Peak-VMS=%s, output-lines=%s, lines/min=%.2f" % (t, gb(rss), gb(vms), lines, (lines * 60.0) / t)

def get_wait_secs(args):
    if len(args) < 2:
        return 0
    if args[0] == '--wait-secs':
        result = int(args[1])
        return result

def run_timed(args):
    start = timer()
    run(args)
    elapsed_time = timer() - start # in seconds
    print("Elapsed time: %.2f secs" % elapsed_time)

ENV = os.environ
prekey = 'pre'

def env(key, missingval):
    return ENV.get(key) if key in ENV else missingval

def gb(x):
    return "%.3f gb"%(float(x) / 1073741824.0)

def nlines(f):
    n = 0
    with open(f) as lines:
        for _ in lines:
            n += 1
    return n

def run(args):
    wait_secs = get_wait_secs(args)
    if wait_secs > 0:
        args = args[2:] # remove the args
    pre = env('pre', "/tmp/%s"%time.time())
    monitor = env('monitor', "0")
    maxepoch = env('maxepoch', '10000000')
    outf = "%s.out.txt" % pre
    outfd = os.open(outf,  os.O_TRUNC | os.O_CREAT | os.O_WRONLY)
    err = open("%s.err.txt" % pre, "w+") if monitor == "0" else None
    sec = open("%s.seconds.txt" % pre, "w")
    LOG.info(" pre=%s out=%s: ls %s*" % (pre, outf, pre))
    LOG.info(' '.join(map(str, args)))

    proc = subprocess.Popen(args, stdout=outfd, stderr=err, close_fds=False)
    p = psutil.Process(proc.pid)
    max_rss = 0
    max_vms = 0
    sum_rss = 0
    sum_vms = 0
    cnt = 0
    interval = int(env('interval', '10'))
    epochbase = float(env('epochbase', '1.2'))
    epochwidth = int(env('epochwidth', '1000'))
    epochlines = int(env('epochlines', '1'))
    LOG.debug("monitor=%s interval=%s maxepoch=%s  epochbase=%s"%(monitor, interval, maxepoch, epochbase))
    wait_times = int(wait_secs / interval)
    LOG.debug('Will wait for {0} secs ({1} times) before probing memory'.format(wait_secs, wait_times))
    peaks = []
    epoch = 1
    lastepoch = 0
    while True:
        rss, vms = p.get_memory_info()
        proc.poll()
        if proc.returncode is not None:
            break
        if cnt > wait_times:
            print("{0}\t{1}".format(rss, vms))
            sys.stdout.flush()
            sec.write("%s\t%s\t%s"%(cnt, rss, vms))
            sec.flush()
            max_rss = max(rss, max_rss)
            max_vms = max(vms, max_vms)
            sum_rss += rss
            sum_vms += vms
        time.sleep(interval)
        cnt += 1
        if cnt == epoch:
            peak = (cnt * interval, rss, vms, nlines(outf))
            LOG.info(" outf=%s %s"%(outf, peakstr(peak)))
            peaks.append(peak)
            epoch = max(epoch + 1, int(epoch * epochbase))
            epoch = min(epoch, maxepoch)
            if epoch - lastepoch > epochwidth:
                epoch += epochwidth - (epoch % epochwidth)

    print("Resident memory max: %.2f GB" % (int(max_rss) / 1073741824.0), file=sys.stderr)
    print("                avg: %.2f GB" % ((int(sum_rss) / cnt) / 1073741824.0), file=sys.stderr)
    print("Virtual memory max: %.2f GB" % (int(max_vms) / 1073741824.0), file=sys.stderr)
    print("               avg: %.2f GB" % ((int(sum_vms) / cnt) / 1073741824.0), file=sys.stderr)
    if err is not None:
        err.seek(0)
        for line in err:
            index = line.find("peak virtual memory = ")
            if index > 0:
                line.rstrip()
                print(line[index:])
    for peak in peaks:
        print(peakstr(peak))
    print("Return code: %d" % proc.returncode, file=sys.stderr)

def main(argv):
    run_timed(argv)

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    LOG.debug("Enabled debug logging")
    main(sys.argv[1:])
