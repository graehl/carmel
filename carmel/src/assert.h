#ifndef ASSERT_H
#define ASSERT_H 1
#include "config.h"
#ifdef DEBUG
#include <assert.h>
#define Assert(a) assert(a)
#else
#define Assert(a)
#endif
#endif
