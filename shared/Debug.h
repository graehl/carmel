// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;; 

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>

#ifndef __NO_GNU_NAMESPACE__
using namespace __gnu_cxx;
#endif
using namespace std;

//! Debug: This is a class to print out debugging information
/*! This should be used for most communication to the user. The reason for this
  is to make sure that all of our debugging messages are consistent (esp
  important for perl readability), and that if for some reason we want to
  redirect error/warning messages to different files, this can be done easily.
*/

class Debug {
public:
  Debug() : debugOS(cerr), infoOS(cout) {};
  
  inline ostream &getDebugOutput() {			   //!< Get the strream to which debugging output is written
    return debugOS;
  }

  inline ostream &getInfoOutput() { 			   //!< Get the stream to which informational output is written
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
  
  ostream &debugOS;					   //!< output stream where error/WARNING messages are sent
  ostream &infoOS;					   //!< output stream where debugging information is sent
};

#endif

