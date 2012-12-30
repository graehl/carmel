#!/usr/bin/python

# python 2.7 includes argparse, or you can install it.

# files are opened in binary mode to prevent later versions from doing auto-unicode

import sys, os, re

def chomp(s):
    return s.rstrip('\r\n')

def log(s,out=sys.stderr):
    out.write("### "+s+"\n")

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


def addarg(argparser,short,typeclass,dest,helptext=None,*L,**M):
    longarg='--'+dest.replace('_','-')
    shortl=[short] if short else []
    L=shortl+[longarg]+list(L)
    argparser.add_argument(*L,dest=dest,type=typeclass,help=helptext,**M)

def wordlines(infile):
    return [chomp(l) for l in infile]

def lenbetween(words,minlen=0,maxlen=None):
    return [x for x in words if len(x)>=minlen and (maxlen is None or len(x)<=maxlen)]

import argparse


parser=argparse.ArgumentParser(description='generate lines using random words from dict file')
addarg(parser,'-d',str,'dictionary','input words file (like /usr/share/dict/words, one word per line)',metavar='FILE')
addarg(parser,'-o', str, 'output', 'output file')
addarg(parser,'-s', str, 'space', 'insert this string between words');
addarg(parser,'-w',str,'word','supplements the word list from input with the given word',nargs='*')
addarg(parser,'-p',float,'pword','probability of choosing from word instead of from dict')
addarg(parser,'-l',int,'lines', 'number of lines to generate')
addarg(parser,'-c',int,'cols', 'number of columns (minimum) per line to generate')
addarg(parser,'-m',int,'maxlen','discard words over this many chars long')
parser.set_defaults(dictionary='-', output='-', word=['\x11'], space='', pword=.2, cols=70, lines=1000, maxlen=5)

import random

if __name__ == '__main__':
    opt=parser.parse_args()
    def shorter(words):
        return lenbetween(words,1,opt.maxlen)
    words=shorter(opt.word)
    log('opt.words=%s --word=%s'%(words,opt.word))
    dicts=shorter(wordlines(open_in(opt.dictionary)))
    out=open_out(opt.output)
    log('using %s words from dictionary %s'%(len(dicts),opt.dictionary))
    pword=opt.pword if len(words) else 0
    if not len(dicts):
        pword=1
    log('with prob=%s, drawing from -word list %s instead of dictionary'%(pword,words))
    cols=opt.cols
    space=opt.space
    if (len(dicts)+len(words)<0):
       raise '-w and -d both empty'
    for _ in range(opt.lines):
        col=0
        while(col<cols):
            seq=words if random.random()<pword else dicts
            w=(space if col>0 else '')+random.choice(seq)
            col+=len(w)
            out.write(w)
        out.write('\n')
