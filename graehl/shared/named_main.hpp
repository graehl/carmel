// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

 allow for argv[0] (else argv[1]) selection of lowercased named main fns -
 compile your mains all at once to share some common symbols (while still
 allowing static link)
*/

#ifndef GRAEHL_SHARED__NAMED_MAIN_HPP
#define GRAEHL_SHARED__NAMED_MAIN_HPP
#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <graehl/shared/lc_ascii.hpp>

#ifndef GRAEHL_NAMED_SINGLE_MAIN
#define GRAEHL_NAMED_SINGLE_MAIN 0
#endif

namespace graehl {

typedef int (*main_fn_t)(int argc, char* argv[]);

typedef std::map<std::string, main_fn_t> named_mains_t;
typedef std::vector<std::string> original_main_names_t;

static original_main_names_t g_original_main_names;
static named_mains_t g_named_mains;

inline void register_named_main(std::string name, main_fn_t fn) {
  g_original_main_names.push_back(name);
  g_named_mains[graehl::lc_ascii_inplace(name)] = fn;
}

struct register_main_fn {
  register_main_fn(std::string const& name, main_fn_t fn) { graehl::register_named_main(name, fn); }
};

template <class MainClass>
struct register_main_class {
  static int run_main_fn(int argc, char* argv[]) {
    MainClass m;
    return m.run_main(argc, argv);
  }
  explicit register_main_class(std::string const& name) { graehl::register_named_main(name, &run_main_fn); }
};

#define GRAEHL_REGISTER_NAMED_MAIN_FN(name, main_fn) graehl::register_main_fn register_##name(#name, main_fn);

#define GRAEHL_REGISTER_NAMED_MAIN(name, main_class) \
  graehl::register_main_class<main_class> register_##name(#name);

#if GRAEHL_NAMED_SINGLE_MAIN
#define GRAEHL_NAMED_MAIN_FN(name, main_fn) \
  int main(int argc, char* argv[]) { return main_fn(argc, argv); }
#define GRAEHL_NAMED_MAIN(name, main_class) \
  int main(int argc, char* argv[]) {        \
    main_class m;                           \
    return m.run_main(argc, argv);          \
  }
#else
#define GRAEHL_NAMED_MAIN_FN(name, main_fn) GRAEHL_REGISTER_NAMED_MAIN_FN(name, main_fn)
#define GRAEHL_NAMED_MAIN(name, main_class) GRAEHL_REGISTER_NAMED_MAIN(name, main_class)
#endif

/**
   return 0 if name has only one word (by camelcase or _ or -) else position of last word
*/
inline unsigned last_word_camelcase_starts(char const* s, unsigned len) {
  if (len && --len)
    while (--len) {
      char c = s[len];
      if (c == '_') return len + 1;
      if (c == '-') return len + 1;
      if (c >= 'A' && c <= 'Z') return len;
    }
  return 0;
}

/**
   run g_named_mains[name] where name is argv[0], else last_word_camelcase(argv[0]), else argv[1]
*/
inline int run_named_main(int argc, char* argvin[], bool tryLastWord = true,
                          std::ostream* usage_if_not_found = &std::cerr, int exitcode_if_not_found = 1) {
  if (argc > 0) {
    char** argv = argvin;
    named_mains_t::const_iterator f;
    for (int argi = 0; argi < 2; ++argi) {
      char const* arg = *argv;
      std::string lcname;
      append_lc_ascii(lcname, arg);
      if ((f = g_named_mains.find(lcname)) != g_named_mains.end()) return (*f->second)(argc, argv);
      if (!argi) {
        unsigned starts = last_word_camelcase_starts(arg, lcname.size());
        if (starts) {
          lcname.erase(0, starts);
          if ((f = g_named_mains.find(lcname)) != g_named_mains.end()) return (*f->second)(argc, argv);
        }
      }
      --argc;
      if (!argc) break;
      ++argv;
    }
  }

  if (usage_if_not_found)
    if (g_named_mains.empty())
      *usage_if_not_found << "ERROR: no commands defined by programmer\n";
    else {
      *usage_if_not_found << "last word of executable name (" << argvin[0]
                          << ") else first argument indicate a subcommand (case insensitive).\n";
      *usage_if_not_found << "\ntry " << argvin[0] << " [subcommand] --help for subcommand usage:\n\n";
      for (original_main_names_t::const_iterator i = g_original_main_names.begin(),
                                                 e = g_original_main_names.end();
           i != e; ++i)
        *usage_if_not_found << *i << "\n";
    }
  return exitcode_if_not_found;
}


}

#endif
