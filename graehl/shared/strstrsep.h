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
#ifndef STRSTRSEP_H
#define STRSTRSEP_H

char *strstrsep(char **stringp, const char *delim);
char *strsep_(char **stringp, const char *delims);

/**
   strsep(stringp, " \t\n").
*/
inline char* strsepspaces(char **stringp) {
  char* s;
  if ((s = *stringp) == NULL) return NULL;
  char c;
  for (char *tok = s;;) {
    c = *s++;
    if (!c) {
      *stringp = NULL;
      return tok;
    } else if (c == ' ' || c == '\n' || c == '\t') {
      s[-1] = 0;
      *stringp = s;
      return tok;
    }
  }
  /* NOTREACHED */
}

static inline char *unstrstr(char *lasttok, char *begin) {
  while (--lasttok >= begin) {
    if (*lasttok == 0)
      *lasttok = ' ';
  }
  return begin;
}


#endif
