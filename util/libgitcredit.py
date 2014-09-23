### from gitcreditlib import *


import sys, os, re, subprocess

from itertools import imap

def dict2str(d, pairsep=',', kvsep=':', omitv=None):
    return pairsep.join('%s%s%s' % (k, kvsep, v) for k, v in d.iteritems() if omitv is None or v != omitv)

def chomp(s):
    return s.rstrip('\r\n')

def chomped_lines(lines):
    return imap(chomp, lines)  # python 2.x map returns a list, not a generator. imap returns a generator

def is_terminal_fname(fname):
    "return whether fname is '-' or '' or none - for stdin or stdout"
    return (fname is None) or fname=='-' or fname==''

import gzip

def open_in(fname):
    "if fname is '-', return sys.stdin, else return open(fname,'rb') (or if fname ends in .gz, gzip.open it)"
    return sys.stdin if is_terminal_fname(fname) else (gzip.open if fname.endswith('.gz') else open)(fname,'rb')

def maybe_lines_in(fname):
    if fname:
        return map(chomp, open_in(fname))
    else:
        return []

import errno

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as e:
        pass
        if getattr(e, 'errno') == errno.EEXIST:
            pass
        else: raise

def mkdir_parent(file):
    mkdir_p(os.path.dirname(file))

def open_out(fname, append=False, mkdir=False):
    """if fname is '-' or '' or none, return sys.stdout, else return open(fname,'w').
      not sure if it's ok to close stdout, so let GC close file please."""
    if is_terminal_fname(fname):
        return sys.stdout
    if mkdir:
        mkdir_parent(fname)
    return (gzip.open if fname.endswith('.gz') else open)(fname,'b'+'a' if append else 'w')

def log(s, out=sys.stderr):
    out.write("### %s\n" % s)

def writeln(line, out=sys.stdout):
    out.write(line)
    out.write('\n')

import argparse

class hexint_action(argparse.Action):
    'An argparse.Action that handles hex string input'
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, int(values, 0))
    pass

def addarg(argparser, shortname, typeclass, dest, help=None, action=None, *L, **M):
    """helper for argparse (part of python 2.7, or you can install it)

    example usage:

args = argparse.ArgumentParser(description='generate lines using random words from dict file')

addarg(args, '-d', str, 'dictionary', 'input words file', metavar='FILE')
addarg(args, '-w', str, 'word', 'supplements the word list from input with the given word', nargs='*')

args.set_defaults(dictionary='-')

    """
    longarg = '--' + dest.replace('_', '-')
    shortl = [shortname] if shortname else []
    L = shortl + [longarg] + list(L)
    if action is None and typeclass == int:
        action = hexint_action
        typeclass = str
    argparser.add_argument(*L, dest=dest, type=typeclass, help=help, action=action, **M)

def addpositional(argparser, dest, help=None, nargs='*', option_strings=[], metavar='FILE', typeclass=str, **M):
    argparser.add_argument(option_strings=option_strings, dest=dest, nargs=nargs, metavar=metavar, help=help, type=typeclass, **M)

def opposite_store_bool(action):
    store_true = 'store_true'
    return store_true if action == store_true else 'store_false'

def addflag(argparser, shortname, dest, help=None, action='store_true', **M):
    addarg(argparser, shortname, dest, action=action, **M)
