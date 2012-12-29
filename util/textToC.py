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

digitChars=set('0123456789')
def varname_from(s):
    r=re.sub(r'[^a-zA-Z0-9_]+','_',s)
    if r[0] in digitChars:
        return '_'+r
    return r


import argparse

parser=argparse.ArgumentParser(description='translate (utf8) input file lines to C header files (without include guards or namespaces)')
parser.add_argument('inputs', metavar='FILE', type=str, nargs='*',
                    help='input file(s). filename before "." is the c variable name')
parser.add_argument('-o', '--output', type=str, dest='output',
                    help='output file. defaults to first input file with [--output-extension] appended')
parser.add_argument('-e', '--output-extension', type=str, dest='extension',
                    help='output file extension (added to -o)')
parser.add_argument('-c', '--comment', dest='comment', type=bool,
                    help='add comment line for each input file')
parser.add_argument('-k', '--keep-newlines', dest='keepNewlines', type=bool,
                    help='for lines,  keep newlines')
parser.add_argument('-l', '--lines', action='store_true', dest='lines',
                    help='instead of a single string for an input file inputFile.ext: char const* inputFile[]={"line 1", "line 2"}; unsigned const inputFileLines=2;')
parser.add_argument('-s', '--single', action='store_false', dest='lines',
                    help='single string for an input file inputFile.ext: char const* inputFile={"line 1\nline 2\n"};')
parser.add_argument('-p', '--prefix', metavar='prefix', type=str, dest='prefix',
                    help='prefix input file name with this to form c variable name');

parser.set_defaults(inputs=['-'], output=None, lines=True, prefix='', extension='.h', comment=True, keepNewlines=False)
args=parser.parse_args()

mustEscChars='"\\'
escChars={r'"':r'\"',
          '\\':r'\\',
          '\n':r'\n',
          '\t':r'\t',
      }
def hexNonPrint(byte):
    "need to do preprocessor string concatenation because \xabcd would be hex char 0xABCD,  not 0xAB followed by ascii CD. see http://stackoverflow.com/questions/5784969/when-did-c-compilers-start-considering-more-than-two-hex-digits-in-string-lite"
    return ('\\x%02x""'%byte) if (byte<32 or byte>=128) else chr(byte)

def cchar(char):
    return escChars.get(char, hexNonPrint(ord(char)))

def cstr(bytestr):
    return '"'+''.join(map(cchar, bytestr))+'"'


inputs=args.inputs if args.inputs else ['-']
ibases=map(basename, inputs)
if len(set(ibases)) < len(ibases):
    log("WARNING: duplicate input file basenames will result in duplicate variable names: %s"%inputs)

output=args.output if args.output else inputs[0]
if output != '-':
    output+='.h'

log(" > %s"%output)

outf=open_out(output)
explain=(' lines array %s original newlines'%('with' if args.keepNewlines else 'without')) if args.lines else ''
array='[]' if args.lines else ''
for iname in inputs:
    ifile=open_in(iname)
    inbytes=ifile.read()
    ibase=basename(iname)
    varname=varname_from(args.prefix+stripext(ibase))
    decl='char const* %s%s = '%(varname, array)
    log(decl)
    if args.comment:
        outf.write("/** contents of file '%s' (auto-generated%s) */\n"%(ibase, explain))
    outf.write(decl)
    if args.lines:
        outf.write('{\n  ')
        cstrs=map(cstr, inbytes.splitlines(args.keepNewlines))
        outf.write(',\n  '.join(cstrs))
        outf.write('\n};\n')
        outf.write('unsigned const %sLines = %s'%(varname, len(cstrs)))
    else:
        outf.write('%s'%cstr(inbytes))
    outf.write(';\n\n')
