#include <graehl/shared/warning_compiler.h>
#ifdef _MSC_VER
# pragma warning( pop )
#elif defined(__clang__)
# pragma clang diagnostic pop
#elif HAVE_DIAGNOSTIC_PUSH
# pragma GCC diagnostic pop
#endif
