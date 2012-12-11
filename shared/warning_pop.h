#ifdef _MSC_VER
#pragma warning( pop )
#else
#include "warning_compiler.h"
#ifdef __clang__
# pragma clang diagnostic pop
#elif HAVE_DIAGNOSTIC_PUSH
# pragma GCC diagnostic pop
#endif
#endif
