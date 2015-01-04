/** \file

 .
*/

#ifndef STATIC_FGETS_BUF_JG_2014_12_31_H
#define STATIC_FGETS_BUF_JG_2014_12_31_H
#pragma once

#ifndef READ_BUFSIZE
#define READ_BUFSIZE (8 * 1024 * 1024)
#endif

#ifndef FGETS_UNLOCKED
#if _GNU_SOURCE
#define FGETS_UNLOCKED fgets_unlocked
#else
#define FGETS_UNLOCKED fgets
#endif
#endif

static char buf[READ_BUFSIZE], bufstdio[READ_BUFSIZE];
#ifndef FALSE_SHARING_PROTECT
#define FALSE_SHARING_PROTECT 72
#endif

static inline void set_static_bufstdio(FILE *fp) {
  setvbuf(fp, bufstdio + FALSE_SHARING_PROTECT, _IOFBF, READ_BUFSIZE - FALSE_SHARING_PROTECT);
}

#endif
