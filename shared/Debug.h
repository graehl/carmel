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

#define IF_INFO(level) if(INFO_LEVEL>=level)
#define IF_ASSERT(level) if(ASSERT_LEVEL>=level)
#define UNLESS_INFO(level) if(INFO_LEVEL<level)
#define UNLESS_ASSERT(level) if(ASSERT_LEVEL<level)

#define assertlvl(level,assertion) IF_ASSERT(level) {assert(assertion);}



#ifdef NO_INFO
# define DBG_OP_F(lvl,pDbg,op,module,msg,file,line)
#else
# define DBG_OP_F(lvl,pDbg,op,module,msg,file,line) do {        \
        if (INFO_LEVEL >= lvl && (pDbg)->runtime_info_level >= lvl) {   \
   ostringstream os; \
   os << msg; \
   (pDbg)->op(module,os.str(),file,line);       \
} } while(0)
#endif

//!< Q: no FILE/LINE included in output
//!< L: specify verbosity level as first argument
#define DBG_OP(pDbg,op,module,msg) DBG_OP_L(0,pDbg,op,module,msg)
#define DBG_OP_Q(pDbg,op,module,msg) DBG_OP_LQ(0,pDbg,op,module,msg)
#define DBG_OP_L(lvl,pDbg,op,module,msg) DBG_OP_F(lvl,pDbg,op,module,msg,__FILE__,__LINE__)
#define DBG_OP_LQ(lvl,pDbg,op,module,msg) DBG_OP_F(lvl,pDbg,op,module,msg,"",0)
#define NESTINFO_GUARD(pDbg) ns_decoder_global::Debug::Nest debug_nest_guard_ ## __LINE__ (pDbg)

// some of these names might be used, e.g. in windows.h, but seems ok on linux.  compilation will fail/warn if they are in conflict.
#define NESTINFO NESTINFO_GUARD(dbg)
#define INFO(module,msg) DBG_OP(dbg,info,module,msg)
#define WARNING(module,msg) DBG_OP(dbg,warning,module,msg)
#define ERROR(module,msg) DBG_OP(dbg,error,module,msg)
#define FATAL(module,msg) DBG_OP(dbg,fatalError,module,msg)
#define INFOQ(module,msg) DBG_OP_Q(dbg,info,module,msg)
#define WARNINGLQ(lvl,module,msg) DBG_OP_LQ(lvl,dbg,warning,module,msg)
#define WARNINGL(lvl,module,msg) DBG_OP_L(lvl,dbg,warning,module,msg)
#define WARNINGQ(module,msg) DBG_OP_Q(dbg,warning,module,msg)
#define ERRORQ(module,msg) DBG_OP_Q(dbg,error,module,msg)
#define FATALQ(module,msg) DBG_OP_Q(dbg,fatalError,module,msg)
#define INFOLQ(lvl,module,msg) DBG_OP_LQ(lvl,dbg,info,module,msg)
#define INFOL(lvl,module,msg) DBG_OP_L(lvl,dbg,info,module,msg)
#ifdef TEST
#define INFOT(msg) DBG_OP(&test_dbg,info,"TEST",msg)
#define WARNT(msg) DBG_OP(&test_dbg,warning,"TEST",msg)
#define NESTT NESTINFO_GUARD(&test_dbg)
#else
#define INFOT(msg) 
#define WARNT(msg) 
#define NESTT
#endif

namespace ns_decoder_global {

  //! Debug: This is a class to print out debugging information
  /*! This should be used for most communication to the user. The reason for this
    is to make sure that all of our debugging messages are consistent (esp
    important for perl readability), and that if for some reason we want to
    redirect error/warning messages to different files, this can be done easily.
  */

  class Debug {
  public:
      int runtime_info_level;
      unsigned info_outline_depth;
      void increase_depth() {
          ++info_outline_depth;
      }
      void decrease_depth() {
          if (info_outline_depth == 0)
              warning("Debug","decrease_depth called more times than increase_depth - clamping at 0");
          else
              --info_outline_depth;          
      }
      struct Nest {
          Debug *dbg;
          Nest(Debug *_dbg) : dbg(_dbg) {
              dbg->increase_depth();
          }
          Nest(const Nest &o) : dbg(o.dbg) {
              dbg->increase_depth();
          }
          ~Nest() {
              dbg->decrease_depth();
          }
      };
      
      Debug() : runtime_info_level(INFO_LEVEL), debugOS(&cerr), infoOS(&cerr) {}

    inline ostream &getDebugOutput() {                     //!< Get the strream to which debugging output is written
      return *debugOS;
    }

    inline ostream &getInfoOutput() {                      //!< Get the stream to which informational output is written
      return *infoOS;
    }
    inline void setDebugOutput(ostream &o) {                     //!< Set the strream to which debugging output is written
      debugOS=&o;
    }

    inline void setInfoOutput(ostream &o) {                      //!< Set the stream to which informational output is written
      infoOS=&o;
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
        const char OUTLINE_CHAR='*';
        for (unsigned depth=info_outline_depth;depth>0;--depth)
            getInfoOutput() << OUTLINE_CHAR;
      if (file=="") {
        getInfoOutput() << module << ": " << info << endl;
      } else {
        getInfoOutput() << module << "(" << file << ":" << line << "): " << info << endl;
      }
    }
      void set_info_level(int lvl) {
          runtime_info_level=lvl;
      }
      int get_info_level() const {
          return runtime_info_level;
      }
  private:

    ostream *debugOS;                                      //!< output stream where error/WARNING messages are sent
    ostream *infoOS;                                       //!< output stream where debugging information is sent
  };
}

#ifdef TEST
# ifdef MAIN
ns_decoder_global::Debug test_dbg;
# endif 
#endif 


#endif

