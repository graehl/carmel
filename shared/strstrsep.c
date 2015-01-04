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
