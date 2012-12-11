#ifndef GRAEHL__SHARED__GZSTREAM_HPP
#define GRAEHL__SHARED__GZSTREAM_HPP

#include <graehl/shared/gzstream.h>
#if (!defined(GRAEHL__NO_GZSTREAM_MAIN) && defined(GRAEHL__SINGLE_MAIN)) || defined(GRAEHL__GZSTREAM_MAIN)
//FIXME: generate named library/object instead?
# include "gzstream.cpp"
#endif


#endif
