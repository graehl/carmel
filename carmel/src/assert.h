#ifndef ASSERT_H
#define ASSERT_H 1
#include "config.h"
#include <cassert>
#ifdef DEBUG
#define Assert(a) assert(a)
#else
#define Assert(a)
#endif
#endif
