#ifndef ASSERT_H
#define ASSERT_H 1
#include "config.h"
#include <cassert>
#ifdef DEBUG
#define Assert(a) assert(a)
#define Paranoid(a) do { a; } while (0)
#else
#define Assert(a)
#define Paranoid(a)
#endif
#endif
