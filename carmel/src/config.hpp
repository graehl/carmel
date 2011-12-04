#ifndef CARMEL__CONFIG_HPP
#define CARMEL__CONFIG_HPP

//#define USE_OPENFST

#ifdef USE_OPENFST
// set this only if you add -I$(OPENFST)/src and have installed headers or also -I$(OPENFST)/src/include
# include <cstring>
# include "fst/compat.h"
# include <stdexcept>

namespace graehl {
struct log_message_exception : public LogMessage
{
    bool exception;
    log_message_exception(std::string const& type) : LogMessage(type=="FATAL"?"FATAL EXCEPTION":type)
    {
        exception=(type=="FATAL");
    }
    ~log_message_exception()
    {
        if (exception)
            throw std::runtime_error("openfst FATAL exception");
    }

};

}

# undef LOG
# define LOG(type) graehl::log_message_exception(#type).stream()

# include "fst/vector-fst.h"

# ifdef GRAEHL__SINGLE_MAIN
// these header are only used in carmel.cc:
#include "fst/minimize.h"
#include "fst/rmepsilon.h"
#include "fst/determinize.h"
#include "fst/connect.h"

// this saves us linking to a separately built lib:
#include "lib/util.cc"
#include "lib/fst.cc"
#include "lib/flags.cc"
#include "lib/properties.cc"
#include "lib/symbol-table.cc"
#include "lib/compat.cc"
#endif

#endif

// allows WFST to be indexed in either direction?  not recommended. or tested lately.
//#define BIDIRECTIONAL

#endif
