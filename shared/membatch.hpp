#ifndef MEMBATCH_HPP
#define MEMBATCH_HPP

#include <string>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "fileargs.hpp"

#if !defined( IO_WINDOWS ) && !defined( IO_POSIX )
# if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#  define IO_WINDOWS
# else
#  define IO_POSIX
# endif
#endif

/*
    HANDLE handle=::CreateFileA(path.c_str(),GENERIC_WRITE,FILE_SHARE_DELETE,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

*/
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _MSVC_VER
#include <io.h>
#endif
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <boost/lexical_cast.hpp>
#include <string>

bool create_file(const std::string& path,std::size_t size) {
#ifdef _WIN32
#if 0
    //VC++ only, unfortunately
    int fd=::_open(path.c_str(),_O_CREAT|_O_SHORT_LIVED);
    if (fd == -1)
        return false;
    if (::_chsize(fd,size) == -1)
        return false;
    return ::_close(fd) != -1;
#else
    HANDLE fh=::CreateFileA( path.c_str(),GENERIC_WRITE,FILE_SHARE_DELETE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return false;
    if(::SetFilePointer(fh,size,NULL,FILE_BEGIN) != size)
        return false;
    if (!::SetEndOfFile(fh))
        return false;
    return ::CloseHandle(fh);
#endif
#else
    return ::truncate(path.c_str(),size) != -1;
#endif
}



#endif
