// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;;
// ignacio

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <string>
#include <iostream>
#include <sstream>

#ifndef __NO_GNU_NAMESPACE__
using namespace __gnu_cxx;
#endif
using namespace std;

#ifndef INFO_LEVEL
# define INFO_LEVEL 9999
#endif
#ifndef ASSERT_LEVEL
# define ASSERT_LEVEL 9999
#endif

#define IF_INFO(level) if(INFO_LEVEL>level)
#define IF_ASSERT(level) if(ASSERT_LEVEL>level)

#define assertlvl(level,assertion) IF_ASSERT(level) {assert(assertion);}

#ifdef NO_INFO
# define DBG_OP_F(pDbg,op,module,msg,file,line,lvl)
#else
# define DBG_OP_F(pDbg,op,module,msg,file,line,lvl) do {   \
  if (INFO_LEVEL >= lvl) { \
   ostringstream os; \
   os << msg; \
   pDbg->op(module,os.str(),file,line);          \
} } while(0)
#endif

#define DBG_OP(pDbg,op,module,msg) DBG_OP_L(pDbg,op,module,msg,0)
#define DBG_OP_Q(pDbg,op,module,msg) DBG_OP_LQ(pDbg,op,module,msg,0)
#define DBG_OP_L(pDbg,op,module,msg,lvl) DBG_OP_F(pDbg,op,module,msg,__FILE__,__LINE__,lvl)
#define DBG_OP_LQ(pDbg,op,module,msg,lvl) DBG_OP_F(pDbg,op,module,msg,"",0,lvl)

namespace ns_decoder_global {

  //! Debug: This is a class to print out debugging information
  /*! This should be used for most communication to the user. The reason for this
    is to make sure that all of our debugging messages are consistent (esp
    important for perl readability), and that if for some reason we want to
    redirect error/warning messages to different files, this can be done easily.
  */

  class Debug {
  public:
    Debug() : debugOS(cerr), infoOS(cout) {};

    inline ostream &getDebugOutput() {                     //!< Get the strream to which debugging output is written
      return debugOS;
    }

    inline ostream &getInfoOutput() {                      //!< Get the stream to which informational output is written
      return infoOS;
    }

    void error(const string &module, const string &info, const string &file="", const int line=0) { //!< prints an error
      getDebugOutput() << "::" << module << "(" << file << ":" << line << "): ERROR: " << info << endl;
    }

    void fatalError(const string &module, const string &info, const string &file="", const int line=0) { //!< prints an error and dies
      error(module, info, file, line);
      assert(0);
      exit(-1);
    }

    void warning(const string &module, const string &info, const string &file="", const int line=0) { //!< prints a warning message
      getDebugOutput() << "::" << module << "(" << file << ":" << line << "): WARNING: " << info << endl;
    }

    void info(const string &module, const string &info, const string &file="", const int line=0) { //!< prints an informational message
      if (file=="") {
        getInfoOutput() << module << ": " << info << endl;
      } else {
        getInfoOutput() << module << "(" << file << ":" << line << "): " << info << endl;
      }
    }

  private:

    ostream &debugOS;                                      //!< output stream where error/WARNING messages are sent
    ostream &infoOS;                                       //!< output stream where debugging information is sent
  };
}

#endif

