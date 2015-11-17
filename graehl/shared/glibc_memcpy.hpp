#pragma once
/// http://www.win.tue.nl/~aeb/linux/misc/gcc-semibug.html
#if defined(__linux__) && defined(__GNUC__) && defined(__LP64__)  /* only under 64 bit gcc */
#ifndef GRAEHL_NO_GLIBC_MEMCPY_WORKAROUND
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
#endif
#endif
