// ;; -*- mode: C++; fill-column: 80; comment-column: 59; -*-
// ;; 

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>

#ifndef __NO_GNU_NAMESPACE__
using namespace __gnu_cxx;
#endif
using namespace std;

//! Debug: A short description of Debug
/*! A longer description of Debug
*/

class Debug {
public:
  Debug() : debugOS(cerr), infoOS(cout) {};
  
  inline ostream &getDebugOutput() {
    return debugOS;
  }

  inline ostream &getInfoOutput() { 
    return infoOS;
  }
  
  void error(const string &module, const string &info, const string &file="", const int line=0) { // prints an error
    getDebugOutput() << "::" << module << "(" << file << ":" << line << "): ERROR: " << info << endl;    
  }
  
  void fatalError(const string &module, const string &info, const string &file="", const int line=0) { // prints an error and dies
    error(module, info, file, line);
    exit(-1);
  }

  void warning(const string &module, const string &info, const string &file="", const int line=0) { 
    getDebugOutput() << "::" << module << "(" << file << ":" << line << "): WARNING: " << info << endl;    
  }		

  void info(const string &module, const string &info, const string &file="", const int line=0) { 
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

