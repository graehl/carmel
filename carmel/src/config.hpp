#ifndef CARMEL__CONFIG_HPP
#define CARMEL__CONFIG_HPP

#define USE_OPENFST

#ifdef USE_OPENFST
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
#include "fst/lib/util.cc"
#include "fst/lib/fst.cc"
#include "fst/lib/flags.cc"
#include "fst/lib/properties.cc"
#include "fst/lib/symbol-table.cc"
#include "fst/lib/compat.cc"
#endif

#endif

#endif
