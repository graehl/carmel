// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define GRAEHL__SINGLE_MAIN
#include "main.hpp"

#define VERSION "0.1"

//#include "ttable.hpp"
//#include "lexicalfeature.hpp"

#include <boost/program_options.hpp>
#include "fileargs.hpp"

#include "fileheader.hpp"
#include "backtrace.hpp"

using namespace boost;
using namespace std;
using namespace boost::program_options;

inline ostream & write_my_header(ostream &o) {
  return write_header(o) << " add-giza-models version= {{ {" VERSION "}}} ";
}

MAIN_BEGIN
{
  DBP_INC_VERBOSE;
#ifdef DEBUG
  DBP::set_logstream(&cerr);
#endif
  //DBP_OFF;

  Infile in;
  Outfile out;
  bool help;
  int log_level;
  string usage_str="Expects positive integers (ignoring other characters), counting the number of lines on which each occurs.\nOutput by default is one per line counts starting with id 1 on line 1 (change that with --sparse)\nYou may run out of memory if your IDs approach billions.";

  options_description general("General options");
  general.add_options()
      ("help,h", bool_switch(&help), "show usage/documentation")
      ("infile,i", value<Infile>(&in)->default_value(default_in,"STDIN"),"Input file")
      ("outfile,o", value<Outfile>(&out)->default_value(default_out,"STDOUT"),"Output file")
      ;
  options_description cosmetic("Cosmetic options");
  cosmetic.add_options()
      ("log-level,L", value<int>(&log_level)->default_value(1),"Higher log-level => more status messages to logfile (0 means absolutely quiet)")
      ;

  options_description all("Allowed options");

  positional_options_description positional; positional.add("infile", 1);

  all.add(general).add(cosmetic);

  const char *progname = argv[0];
  try {
    variables_map vm;
    command_line_parser cl(argc, argv);
    cl.options(all);
    cl.positional(positional);
    store(cl.run(), vm);
    notify(vm);
#ifdef DEBUG
    DBP::set_logstream(&cerr);
    DBP::set_loglevel(log_level);
#endif

    if (help) {
      cerr << progname << ' ' << "version " << VERSION << ":\n";
      cerr << all << "\n";
      cerr << usage_str << "\n";
      return 0;
    }

    /////////////////////// ACTUAL PROGRAM:


    /////////////////////
  }
  catch(exception& e) {
    cerr << "ERROR: " << e.what() << "\n\n";
    cerr << "Try '" << progname << " -h' for documentation\n";
    BackTrace::print(cerr);
    return 1;
  }
  catch(...) {
    cerr << "FATAL: Exception of unknown type!\n\n";
    return 2;
  }
  return 0;

}
MAIN_END
