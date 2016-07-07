#pragma once
/// http://www.win.tue.nl/~aeb/linux/misc/gcc-semibug.html
#if !defined(__APPLE__) && defined(__linux__) && defined(__GNUC__) && defined(__LP64__) \
    && !defined(USE_LATEST_MEMCPY) /* only under 64 bit gcc */
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
#endif
