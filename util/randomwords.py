#!/usr/bin/python

# python 2.7 includes argparse, or you can install it.

# files are opened in binary mode to prevent later versions from doing auto-unicode

import sys, os, re

def chomp(s):
    return s.rstrip('\r\n')

def log(s, out=sys.stderr):
    out.write("### " + s + "\n")

def stripext(fname):
    return os.path.splitext(fname)[0]

basename=os.path.basename

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        pass
        if exc.errno == errno.EEXIST:
            pass
        else: raise

def mkdir_parent(file):
    mkdir_p(os.path.dirname(file))

import gzip

def is_terminal_fname(fname):
    "return if fname is '-' or '' or none - for stdin or stdout"
    return (fname is None) or fname=='-' or fname==''

def open_in(fname):
    "if fname is '-', return sys.stdin, else return open(fname,'rb') (or if fname ends in .gz, gzip.open it)"
    return sys.stdin if is_terminal_fname(fname) else (gzip.open if fname.endswith('.gz') else open)(fname,'rb')

def open_out(fname, append=False, mkdir=False):
    """if fname is '-' or '' or none, return sys.stdout, else return open(fname,'w').
      not sure if it's ok to close stdout, so let GC close file please."""
    if is_terminal_fname(fname):
        return sys.stdout
    if mkdir:
        mkdir_parent(fname)
    return (gzip.open if fname.endswith('.gz') else open)(fname,'b'+'a' if append else 'w')


import argparse

class hexint_action(argparse.Action):
    'An argparse.Action that handles hex string input'
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, int(values, 0))
    pass

def addarg(argparser, shortname, typeclass, dest, helptext=None, action=None, *L, **M):
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
    argparser.add_argument(*L, dest = dest, type = typeclass, help = helptext, action=action, **M)

def wordlines(infile):
    return [chomp(l) for l in infile]

def lenbetween(words,minlen=0,maxlen=None):
    return [x for x in words if len(x)>=minlen and (maxlen is None or len(x)<=maxlen)]


parser = argparse.ArgumentParser(description='generate lines using random words from dict file (or RNG)')

addarg(parser, '-d', str, 'dictionary', 'input words file (like /usr/share/dict/words, one word per line)', metavar='FILE')
addarg(parser, '-r', int, 'prandom', 'probabiliy of choosing license plate A-Z0-9 random words')
addarg(parser, '-P', int, 'platelen', 'length of license plate')
addarg(parser, '-L', int, 'maxplatelen', 'max length of license plate')
addarg(parser, '-o', str, 'output', 'output file')
addarg(parser, '-s', str, 'space', 'insert this string between words');
addarg(parser, '-p', float, 'pword', 'probability of choosing from word instead of from dict')
addarg(parser, '-l', int, 'lines', 'number of lines to generate')
addarg(parser, '-c', int, 'cols', 'number of columns (minimum) per line to generate')
addarg(parser, '-C', int, 'maxcols', 'if maxcols, randomly choose between [cols, maxcols] as the target')
addarg(parser, '-m', int, 'maxlen', 'discard words over this many chars long')
addarg(parser, '-u', int, 'unicode', 'for license plates, first unicode codepoint as an int, e.g. 0x0600 for start of basic arabic')
addarg(parser, '-U', int, 'nunicode', 'for license plates, number of unicode chars, e.g. 256 for basic arabic. 0 means use A-Z0-9')
addarg(parser, '-R', int, 'repeats', 'repeat each line this many times')
addarg(parser, '-w', str, 'word', 'supplements the word list from input with the given word', nargs='*')
parser.set_defaults(dictionary='', output='-', word=['\x11'], space=' ', pword=.2, cols=70, lines=1000, maxlen=20, platelen=33, prandom=0, unicode=0x600, nunicode=256, maxplatelen=0, maxcols=0, repeats=1)

import random
import string

def ranbetween(l, u):
    return random.randint(l, u) if u > l else l


def utf8u(u):
    return unichr(u).encode('utf-8')

def randomplate(len=33, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(len))

def withprob(p=1):
    return p >= 1 or (p > 0 and random.random() < p)

if __name__ == '__main__':
    opt=parser.parse_args()
    def shorter(words):
        return lenbetween(words,1,opt.maxlen)
    words=shorter(opt.word)
    log('opt.words=%s --word=%s'%(words,opt.word))
    if len(opt.dictionary):
        dicts = shorter(wordlines(open_in(opt.dictionary)))
    else:
        dicts = []
    out=open_out(opt.output)
    log('using %s words from dictionary %s'%(len(dicts),opt.dictionary))
    pword=opt.pword if len(words) else 0
    log('with prob=%s, drawing from -word list %s instead of dictionary'%(pword,words))
    space = opt.space
    prandom = opt.prandom
    if (len(dicts)+len(words)<0):
       raise '-w and -d both empty'
    spaces = ['', space]
    utf8s = [utf8u(opt.unicode + u) for u in range(opt.nunicode)] if opt.nunicode > 0 else [c for c in string.ascii_uppercase + string.digits]
    log('nunicode=%s chars starting at unicode=%s: %s' % (opt.nunicode, opt.unicode, utf8s))
    if not len(dicts) and prandom < 1:
        pword = 1
    if prandom > 0:
        log('with prob=%s, creating random license plate of length %s - %s' % (prandom, opt.platelen, opt.maxplatelen))
    for _ in xrange(opt.lines):
        col = 0
        cols = ranbetween(opt.cols, opt.maxcols)
        words = []
        while(col < cols):
            if withprob(prandom):
                word = randomplate(ranbetween(opt.platelen, opt.maxplatelen), utf8s)
            else:
                word = random.choice(words if withprob(pword) else dicts)
            w = spaces[col > 0] + word
            col += len(w)
            words.append(w)
        for _ in range(opt.repeats):
            out.write(''.join(words)+'\n')
