#ifndef OS_HPP
#define OS_HPP

#if !defined( BOOST_IO_WINDOWS ) && !defined( BOOST_IO_POSIX )
# if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#  define BOOST_IO_WINDOWS
#  ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0500
#  endif
# else
#  define BOOST_IO_POSIX
# endif
#endif


#ifdef BOOST_IO_WINDOWS
# define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
# include <windows.h>
 typedef DWORD Error;
#else
# include <errno.h>
# include <string.h>
 typedef int Error;
#endif

#include <string>
#include <cstdio>

bool remove_file(const std::string &filename) {
    return 0==remove(filename.c_str());
}

Error last_error() {
#ifdef BOOST_IO_WINDOWS
    return ::GetLastError();
#else
    return errno;
#endif
}

std::string error_string(Error err) {
#ifdef BOOST_IO_WINDOWS
    LPVOID lpMsgBuf;
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL ) == 0)
        throw std::runtime_error("couldn't generate Windows error message string");
    std::string ret((LPTSTR) lpMsgBuf);
    ::LocalFree(lpMsgBuf);
    return ret;
#else
    return strerror(err);
#endif
}

std::string last_error_string() {
    return error_string(last_error());
}

#endif
