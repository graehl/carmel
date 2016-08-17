/** \file

 like 'backtrace' in gdb

 (linux only so far)
*/

#ifndef STACKTRACE_GRAEHL_2016_08_29_HPP
#define STACKTRACE_GRAEHL_2016_08_29_HPP
#pragma once

#include <iostream>

#ifdef __linux__
#include <execinfo.h>
#include <signal.h>
#endif

namespace graehl {

static const int MAX_TRACE_DEPTH = 64;

void stacktrace(std::ostream& o = std::cerr) {
#ifdef __linux__
  void* trace[MAX_TRACE_DEPTH];
  int trace_size = ::backtrace(trace, MAX_TRACE_DEPTH);
  char** messages = ::backtrace_symbols(trace, trace_size);
  o << "\n!!Stack backtrace:\n";
  for (int i = 0; i < trace_size; ++i) o << "!! " << messages[i] << '\n';
#endif
}


}

#endif
