#!/usr/bin/env python
usage='''
  pragma_once.py [-i] [files]

  check for include guard (file first preprocessor directive is: #ifndef X then #define X ). if present, add #pragma once.

  assumes initial preprocessor directives aren't indented.

'''

import sys

def log(s, out=sys.stderr):
    out.write("### %s\n" % s)

def writeln(line, out=sys.stdout):
    out.write(line)
    out.write('\n')

import argparse

def addpositional(argparser, dest, help=None, nargs='*', option_strings=[], metavar='FILE', typeclass=str, **M):
    argparser.add_argument(option_strings=option_strings, dest=dest, nargs=nargs, metavar=metavar, help=help, type=typeclass, **M)

def addflag(argparser, shortname, dest, help=None, action='store_true', **M):
    argparser.add_argument(shortname, longoption(dest), dest=dest, action=action, help=help, **M)


### end general graehl.py stuff

import sys, os, re, subprocess, argparse

def options():
    parser = argparse.ArgumentParser(description=usage)
    addflag(parser, '-i', 'inplace', 'update files in-place')
    addpositional(parser, 'filename', '(args X for `git blame X`; added to --filelist file)')
    parser.set_defaults(inplace=False)
    return parser

import fileinput

def remove_namespace(opt):
    if not len(opt.namespace):
        raise usage
    nsline = 'namespace %s {' % opt.namespace
    endns = '}'
    nslinemid = ' ' + nsline
    pres=['::', '(', ' ']
    nscolon = opt.namespace+'::'
    if len(opt.filename) == 0:
        opt.filename = [[]]
    cifndef='#ifndef '
    cdefine='#define '
    for filename in opt.filename:
        guarded = False
        firstpre = True
        guard = None
        onced = False
        for line in fileinput.input(filename, inplace=opt.inplace):
            line = line.rstrip()
            if firstpre and line.startswith(cifndef):
                guard = line[len(cifndef):]
            elif guard is not None and firstpre and line.startswith(cdefine):
                guard2 = line[len(cdefine):]
                if guard == guard2:
                    guarded = True
                firstpre = False
            elif line.startswith('#'):
                firstpre = False
            elif guarded:
                print '#pragma once'
                onced = True
                guarded = False
            print line
        if onced:
            log(filename)

if __name__ == '__main__':
    try:
        opt = options().parse_args()
        remove_namespace(opt)
    except KeyboardInterrupt:
        log("^C user interrupt")
