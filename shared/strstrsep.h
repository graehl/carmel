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
