#!/usr/bin/env python
usage='''
  remove_namespace.py -n namespace [-i] [files]

  remove (exactly formatted) 'namespace X {' and one of a presumed-closing '}}', as well as
  \bX:: (which might catch some collateral classes of same name

'''

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

def addflag(argparser, shortname, dest, help=None, action='store_true', **M):
    addarg(argparser, shortname, dest, action=action, **M)


### end general graehl.py stuff

import sys, os, re, subprocess, argparse

def options():
    parser = argparse.ArgumentParser(description=usage)
    addarg(parser, '-n', str, 'namespace',
           'namespace e.g. xmt')
    addflag(parser, '-i', 'inplace', 'update files in-place')
    addpositional(parser, 'filename', '(args X for `git blame X`; added to --filelist file)')
    parser.set_defaults(namespace='xmt', inplace=False)
    return parser

import fileinput

def remove_namespace(opt):
    if not len(opt.namespace):
        raise usage
    nsline = 'namespace %s {' % opt.namespace
    nslinemid = ' ' + nsline
    nscolon1 = '::%s::' % opt.namespace
    nscolon2 = '(%s::' % opt.namespace
    nscolon3 = ' %s::' % opt.namespace
    endns = '}'
    for filename in opt.filename:
        ns = False
        lastwasendns = False
        for line in fileinput.input([filename], inplace=opts.inplace):
            line = line.rstrip()
            if line == nsline:
                ns = True
                continue
            line2 = line.replace(nslinemid, '')
            if line2 != line:
                ns = True
                line = line2
            line = line.replace(nscolon1, '')
            line = line.replace(nscolon2, '')
            line = line.replace(nscolon3, '')
            lastwasendns = all(x = endns for x in line)
            if lastwasendns:
                ns = False
                line = line[1:]
            print line

if __name__ == '__main__':
    try:
        opt = options().parse_args()
        verbose = opt.verbose
        remove_namespace(opt)
    except KeyboardInterrupt:
        log("^C user interrupt")
