#ifndef ASSERT_H
#define ASSERT_H 1
#include "config.h"
#include <assert.h>
#ifdef DEBUG
#define Assert(a) assert(a)
#else
#define Assert(a)
#endif
#endif
