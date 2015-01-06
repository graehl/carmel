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
#include <string.h>
#include <stdlib.h>
#include "strstrsep.h"

/* Like strsep(3) except that the delimiter is a string, not a set of characters.
*/
char* strstrsep(char** stringp, const char* delim) {
  char* match, *save;
  save = *stringp;
  if (*stringp == NULL) return NULL;
  match = strstr(*stringp, delim);
  if (match == NULL) {
    *stringp = NULL;
    return save;
  }
  *match = '\0';
  *stringp = match + strlen(delim);
  return save;
}


/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char* strsep_(char **stringp, const char* delim) {
  char* s;
  const char* spanp;
  char c, sc;
  char* tok;

  if ((s = *stringp) == NULL) return NULL;
  for (tok = s;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
        if (c == 0)
          s = NULL;
        else
          s[-1] = 0;
        *stringp = s;
        return tok;
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}

extern inline char* strsepspaces(char ** stringp);
