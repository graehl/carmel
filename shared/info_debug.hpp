#ifndef INFO_DEBUG_H_inc
#define INFO_DEBUG_H_inc

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "assertlvl.hpp"
#include "breakpoint.hpp"

#ifndef __NO_GNU_NAMESPACE__
using namespace __gnu_cxx;
#endif
using namespace std;

//#undef INFO_LEVEL
#ifndef INFO_LEVEL
# define INFO_LEVEL 9999
#endif

#define COND_INFO(level,op) (INFO_LEVEL op level)
#define COND_INFO_RUNTIME(level,op) (INFO_LEVEL op level && ns_info_debug::debug.runtime_info_level op level)
#define COND_INFO_RUNTIME_EQUAL(level) (COND_INFO(level,>=) && ns_info_debug::debug.runtime_info_level == level)
#define IF_INFO_RUNTIME(level) if(COND_INFO_RUNTIME(level,>=))
#define UNLESS_INFO_RUNTIME(level) if(COND_INFO_RUNTIME(level,<))
#define IF_INFO(level) if(COND_INFO(level,>=))
#define UNLESS_INFO(level) if(COND_INFO(level,<))

#include "makestr.hpp"

/* GUIDE TO NAMES:
   NL: newline
   B: begin (returns stream)
   Q: no file/line (quick/quiet)
   L: level (conditioned on INFO_LEVEL and runtime --verbose being at least that level
   E: expression involving os: i.e. MACRO(os << "hi") not MACRO("hi")
*/

#ifdef NO_INFO
# define DBG_OP_F_NL(lvl,Dbg,op,module,oexp,file,line,newline)
# define DBG_OP_F(lvl,Dbg,op,module,oexp,file,line)
#else

# define DBG_OP_F(lvl,Dbg,op,module,oexp,file,line) DBG_OP_F_NL(lvl,Dbg,op,module,oexp,file,line,true)

# define DBG_OP_F_NL(lvl,Dbg,op,module,oexp,file,line,newline) do {     \
        if (INFO_LEVEL >= lvl && (Dbg).runtime_info_level >= lvl) {   \
   ostringstream os; \
   oexp; \
   if(!os) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write " #module " : " #oexp); \
   (Dbg).op(module,os.str(),file,line,newline);      \
} } while(0)
#endif

//!< Q: no FILE/LINE included in output
//!< L: specify verbosity level as first argument
#define DBG_OP(Dbg,op,module,msg) DBG_OP_L(0,Dbg,op,module,msg)
#define DBG_OP_Q(Dbg,op,module,msg) DBG_OP_LQ(0,Dbg,op,module,msg)
#define DBG_OP_L(lvl,Dbg,op,module,msg) DBG_OP_F(lvl,Dbg,op,module,msg,__FILE__,__LINE__)
#define DBG_OP_LQ_NEWLINE(lvl,Dbg,op,module,msg,newline) DBG_OP_F_NL(lvl,Dbg,op,module,msg,"",0,newline)
#define DBG_OP_LQ(lvl,Dbg,op,module,msg) DBG_OP_F(lvl,Dbg,op,module,msg,"",0)
#define NESTINFO_GUARD(Dbg,lvl) ns_info_debug::info_debug::Nest debug_nest_guard_ ## __LINE__ (Dbg,lvl)

#define O_INSERT(msg) os << msg

// some of these names might be used, e.g. in windows.h, but seems ok on linux.  compilation will fail/warn if they are in conflict.
#define NESTINFO NESTINFO_GUARD(ns_info_debug::debug,1)
#define INFO(module,msg) DBG_OP(ns_info_debug::debug,info,module,O_INSERT(msg))
#define WARNING(module,msg) DBG_OP(ns_info_debug::debug,warning,module,O_INSERT(msg))

#define ERROR_LINENO(module,msg) DBG_OP(ns_info_debug::debug,error,module,O_INSERT(msg))
//ERROR too common a name - conflicts?

#define FATAL(module,msg) DBG_OP(ns_info_debug::debug,fatalError,module,O_INSERT(msg))
#define INFOQ(module,msg) DBG_OP_Q(ns_info_debug::debug,info,module,O_INSERT(msg))
#define INFOB(module) ns_info_debug::debug.info_begin(module,__FILE__,__LINE__)
#define WARNINGB(module) ns_info_debug::debug.warning_begin(module,__FILE__,__LINE__)
#define ERRORB(module) ns_info_debug::debug.error_begin(module,__FILE__,__LINE__)
#define INFOBQ(module) ns_info_debug::debug.info_begin(module,__FILE__,0)
#define INFOENDL ns_info_debug::debug.info_endl()
#define WARNINGBQ(module) ns_info_debug::debug.warning_begin(module,__FILE__,0)
#define ERRORBQ(module) ns_info_debug::debug.error_begin(module,__FILE__,0)

#define INFOQSAMELINE(module,msg) DBG_OP_LQ_NEWLINE(0,ns_info_debug::debug,info,module,O_INSERT(msg),false)
#define INFOSTREAM ns_info_debug::debug.info_sameline()
#define INFOSTREAM_NL ns_info_debug::debug.info_startline()
#define WARNINGLQ(lvl,module,msg) DBG_OP_LQ(lvl,ns_info_debug::debug,warning,module,O_INSERT(msg))
#define WARNINGL(lvl,module,msg) DBG_OP_L(lvl,ns_info_debug::debug,warning,module,O_INSERT(msg))
#define WARNINGQ(module,msg) DBG_OP_Q(ns_info_debug::debug,warning,module,O_INSERT(msg))
#define ERRORQ(module,msg) DBG_OP_Q(ns_info_debug::debug,error,module,O_INSERT(msg))
#define FATALQ(module,msg) DBG_OP_Q(ns_info_debug::debug,fatalError,module,O_INSERT(msg))
#define INFOLQ(lvl,module,msg) DBG_OP_LQ(lvl,ns_info_debug::debug,info,module,O_INSERT(msg))
#define INFOLQE(lvl,module,oexp) DBG_OP_LQ(lvl,ns_info_debug::debug,info,module,oexp)
#define INFOL(lvl,module,msg) DBG_OP_L(lvl,ns_info_debug::debug,info,module,O_INSERT(msg))
#if INFO_LEVEL >= 9
#define INF9(module,msg) INFOLQ(9,module,O_INSERT(msg))
#define INF9IN ns_info_debug::debug.increase_depth()
#define INF9OUT ns_info_debug::debug.decrease_depth()
#define INF9NEST NESTINFO_GUARD(ns_info_debug::debug,9)
#else
#define INF9(module,msg)
#define INF9IN
#define INF9OUT
#define INF9NEST
#endif

#if INFO_LEVEL >= 99
#define INF99(module,msg) INFOLQ(99,module,O_INSERT(msg))
#define INF99IN ns_info_debug::debug.increase_depth()
#define INF99OUT ns_info_debug::debug.decrease_depth()
#define INF99NEST NESTINFO_GUARD(ns_info_debug::debug,99)
#else
#define INF99(module,msg)
#define INF99IN
#define INF99OUT
#define INF99NEST
#endif

#if (defined(TEST) && !defined(QUIET_TEST) )
#define INFOT(msg) DBG_OP(test_dbg,info,"TEST",O_INSERT(msg))
#define WARNT(msg) DBG_OP(test_dbg,warning,"TEST",O_INSERT(msg))
#define NESTT NESTINFO_GUARD(test_dbg,1)
#else
#define INFOT(msg) DBG_OP_L(99,ns_info_debug::debug,info,"TEST",O_INSERT(msg))
#define WARNT(msg) DBG_OP_L(99,ns_info_debug::debug,warning,"TEST",O_INSERT(msg))
#if INFO_LEVEL >= 99
#define NESTT NESTINFO_GUARD(ns_info_debug::debug,99)
#else
#define NESTT
#endif

#endif

namespace ns_info_debug {

//! info_debug: This is a class to print out debugging information
/*! This should be used for most communication to the user. The reason for this
  is to make sure that all of our debugging messages are consistent (esp
  important for perl readability), and that if for some reason we want to
  redirect error/warning messages to different files, this can be done easily.
*/

class info_debug {
 private:
    ostream *debugOS;                                      //!< output stream where error/WARNING messages are sent
    ostream *infoOS;                                       //!< output stream where debugging information is sent
 public:
    int runtime_info_level;
    unsigned info_outline_depth;
    unsigned debug_outline_depth;
    void increase_depth() {
        ++info_outline_depth;
    }
    void decrease_depth() {
        if (info_outline_depth == 0)
            warning("info_debug","decrease_depth called more times than increase_depth - clamping at 0");
        else
            --info_outline_depth;
    }
    void increase_debug_depth() {
        ++debug_outline_depth;
    }
    void decrease_debug_depth() {
        if (debug_outline_depth == 0)
            warning("info_debug","decrease_debug_depth called more times than increase_debug_depth - clamping at 0");
        else
            --debug_outline_depth;
    }
    struct Nest {
        info_debug *pdebug;
        unsigned info_req;
        bool active() const {
            return info_req < pdebug->runtime_info_level;
        }
        Nest(info_debug &debug,unsigned info_lvl_required=0) : pdebug(&debug),info_req(info_lvl_required) {
            if (active())
                pdebug->increase_depth();
        }
/*        Nest(const Nest &o) : pdebug(o.pdebug) {
            if (active())
                pdebug->increase_depth();
        }
*/
        ~Nest() {
            if (active())
                pdebug->decrease_depth();
        }
    };

    info_debug() : debugOS(&cerr), infoOS(&cerr),
              runtime_info_level(INFO_LEVEL),
              info_outline_depth(0),debug_outline_depth(0),info_atnewline(true) {}

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
    void error(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an error
        error_begin(module, file, line) << info << endl;
    }

    ostream & error_begin(const string &module, const string &file="", const int line=0)
    {
        if (debugOS==infoOS)
            info_startline();
        else
            sync();
        getDebugOutput() << "ERROR: " << module;
        if (line)
            getDebugOutput() << "(" << file << ":" << line << ")";
        return getDebugOutput() << ": ";
    }

    void fatalError(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an error and dies
        error(module,info,file,line,endline);
        DEBUG_BREAKPOINT;
        assert(0);
        exit(-1);
    }

    ostream & warning_begin(const string &module, const string &file="", const int line=0)
    {
        if (debugOS==infoOS)
            info_startline();
        else
            sync();
        getDebugOutput() << "WARNING: " << module;
        if (line)
            getDebugOutput() << "(" << file << ":" << line << ")";
        return getDebugOutput() << ": ";
    }

    void warning(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints a warning message
        warning_begin(module,file,line) << info << endl;
    }

    bool info_atnewline; // at fresh newline if true, midline if false

    void sync() const
    {
        std::cout.flush();
    }

    inline void info_endl()
    {
        *infoOS << std::endl;
        info_atnewline=true;
    }

    //post: state=midline
    inline ostream &info_sameline() {
        info_atnewline=false;
        return *infoOS;
    }

    //post: state=midline, after printing a fresh newline (if weren't already at one)
    inline ostream &info_startline() {
        sync();
        if (!info_atnewline) {
            *infoOS << std::endl;
        }
        info_atnewline=false;
        return *infoOS;
    }

    //post: state=newline (printing one out if weren't already)
    // this would never be necessary to use if everyone always used info_startline() (except at the very end when closing stream)
    inline ostream &info_endline() {
        if (!info_atnewline) {
            info_endl();
        }
        return *infoOS;
    }

    //!< note: should close this with info_endl() and not just "\n" or endl
    ostream & info_begin(const string &module, const string &file="", const int line=0) { //!< prints an informational message
        const char OUTLINE_CHAR='*';
        info_startline();
        for (unsigned depth=info_outline_depth;depth>0;--depth)
            getInfoOutput() << OUTLINE_CHAR;
        getInfoOutput() << module;

        if (line) {
            getInfoOutput() << "(" << file << ":" << line << ")";
        }
        return getInfoOutput() << ": ";
    }

    void info(const string &module, const string &info, const string &file="", const int line=0,bool endline=true) { //!< prints an informational message
        info_begin(module,file,line) << info;        
        if (endline)
            info_endl();
    }

    void set_info_level(int lvl) {
        runtime_info_level=lvl;
    }
    int get_info_level() const {
        return runtime_info_level;
    }
};
#ifdef SINGLE_MAIN
info_debug debug;
#else 
extern info_debug debug;        //!< interface for debugging output
#endif 
#ifdef TEST
# ifdef TEST_MAIN
info_debug test_dbg;
# else
info_debug test_dbg;
# endif 
#endif

}




// added by Wei Wang.
/*
 * Here is the typical usage for this mixin class.
 * First, include it in the parents of some class FOO
 *
 * class FOO: public OTHER_PARENT, public FOO { ... }
 *
 * Inside FOO's methods use code such as
 *
 *	if (debug(3)) {
 *	   dout() << "I'm feeling sick today\n";
 *	}
 *
 * Finally, use that code, after setting the debugging level
 * of the object and/or redirecting the debugging output.
 *
 *      FOO foo;
 *	foo.debugme(4); foo.dout(cout);
 *
 * LiBEDebugging can also be set globally (to affect all objects of
 * all classes.
 *
 *	foo.debugall(1);
 *
 */
class Debugger
{
public:
    Debugger(unsigned level = 0)
      : nodebug(false), debugLevel(level), debugStream(&cerr) {};

    bool debug(unsigned level)  const  /* true if debugging */
	{ return (!nodebug && (debugAll >= level || debugLevel >= level)); };
    virtual void debugme(unsigned level) { debugLevel = level; };
				    /* set object's debugging level */
    static void debugall(unsigned level) { debugAll = level; };
				    /* set global debugging level */
    unsigned debuglevel() { return debugLevel; };

    virtual ostream &dout() const { return *debugStream; };
				    /* output stream for use with << */
    virtual ostream &dout(ostream &stream)  /* redirect debugging output */
	{ debugStream = &stream; return stream; };

    static unsigned debugall() { return debugAll;}

    bool nodebug;		    /* temporarily disable debugging */
private:
    static unsigned debugAll;	    /* global debugging level */
    unsigned debugLevel;	    /* level of output -- the higher the more*/
    ostream *debugStream;	    /* current debug output stream */
};

#ifdef SINGLE_MAIN
unsigned Debugger::debugAll = 0;
#endif

#endif
