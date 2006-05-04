#ifndef GRAEHL__SHARED__DEBUGGABLE_HPP
#define GRAEHL__SHARED__DEBUGGABLE_HPP

#ifdef GRAEHL__SINGLE_MAIN
# define DEBUGGABLE__SINGLE_MAIN
#endif

#include <iostream>

namespace graehl {

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
class debuggable
{
public:
    debuggable(unsigned level = 0)
        : nodebug(false), debugLevel(level), debugStream(NULL) {};

    bool debug(unsigned level)  const  /* true if debugging */
	{ return (!nodebug && (debugAll >= level || debugLevel >= level)); };
    void debugme(unsigned level) { debugLevel = level; };
				    /* set object's debugging level */
    unsigned debuglevel() { return debugLevel; };

    std::ostream &dout() const { return debugStream ? *debugStream : *default_debugStream; };
				    /* output stream for use with << */
    std::ostream &dout(std::ostream &stream)  /* redirect debugging output */
	{ debugStream = &stream; return stream; };
    
    static unsigned debugall() { return debugAll;}
    static void debugall(unsigned level) { debugAll = level; };
				    /* set global debugging level */
    
    static void set_default_output(std::ostream &o) 
    {
        default_debugStream=&o;
    }
    
    bool nodebug;		    /* temporarily disable debugging */
private:
    static unsigned debugAll;	    /* global debugging level */
    unsigned debugLevel;	    /* level of output -- the higher the more*/
    std::ostream *debugStream;	    /* current debug output stream */
    static std::ostream *default_debugStream;
};

#ifdef GRAEHL__SINGLE_MAIN
unsigned debuggable::debugAll = 0;
std::ostream *debuggable::default_debugStream=&std::cerr;
#endif

}


#endif
