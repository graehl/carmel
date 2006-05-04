// wraps some memory-leak-detection facilities of Microsoft C++.  you're better off just using valgrind.
#ifndef MEMLEAK_HPP
#define MEMLEAK_HPP

#ifndef PLACEMENT_NEW
#define PLACEMENT_NEW new
#endif

#if defined(DEBUG) && defined(_MSC_VER)
#  define MEMDEBUG // link to MSVCRT
#  define _CRTDBG_MAP_ALLOC
#  define CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#  define INITLEAK_DECL _CrtMemState s1, s2, s3;
#  define INITLEAK_DO do { _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE );_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR ); _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );} while(0)
#  define CHECKLEAK(i) do {  _CrtMemCheckpoint( (((i)%2)?&s1:&s2) ); if ((i)>0) { _CrtMemDifference( &s3, (((i)%2)?&s2:&s1), (((i)%2)?&s1:&s2) ); _CrtMemDumpStatistics( &s3 );} } while(0)
#  define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)

# else

#  define NEW new
#  define INITLEAK_DECL
#  define INITLEAK_DO
#  define CHECKLEAK(i)
# endif

#  define INITLEAK INITLEAK_DECL INITLEAK_DO

#endif
