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
