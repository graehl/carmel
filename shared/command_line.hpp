#ifndef GRAEHL__SHARED__COMMAND_LINE_HPP
#define GRAEHL__SHARED__COMMAND_LINE_HPP

#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/shell_escape.hpp>
#include <sstream>
#include <vector>

namespace graehl {

template <class Ch, class Tr>
inline std::basic_ostream<Ch,Tr> & print_command_line(std::basic_ostream<Ch,Tr> &out, int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    if (header)
        out << header;
    WordSeparator<' '> sep;
    for (int i=0;i<argc;++i) {
        out << sep;
        out_shell_quote(out,argv[i]);
    }
    if (header)
        out << std::endl;
    return out;
}

inline std::string get_command_line( int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    std::ostringstream os;
    print_command_line(os,argc,argv,header);
    return os.str();
}

struct argc_argv : private std::stringbuf
{
    typedef char *arg_t;
    typedef arg_t *argv_t;
    
    std::vector<char *> argvptrs;
    int argc() const
    {
        return argvptrs.size();
    }
    argv_t argv() const
    {
        return argc() ? (char**)&(argvptrs[0]) : NULL;
    }
    void parse(const std::string &cmdline) 
    {
        argvptrs.clear();
        argvptrs.push_back("ARGV");
#if 1
        str(cmdline+" ");  // we'll need space for terminating final arg.
#else
        str(cmdline);
        seekoff(0,ios_base::end,ios_base::out);        
        sputc((char)' ');
#endif
        char *i=gptr(),*end=egptr();

        char *o=i;
        char terminator;
    next_arg:
        while(i!=end && *i==' ') ++i;  // [ ]*
        if (i==end) return;

        if (*i=='"' || *i=='\'') {
            terminator=*i;
            ++i;
        } else {
            terminator=' ';
        }
        
        argvptrs.push_back(o);

        while(i!=end) {
            if (*i=='\\') {
                ++i;
            } else if (*i==terminator) {
                *o++=0;
                ++i;
                goto next_arg;
            }
            *o++=*i++;
        }
        *o++=0;
    }
    argc_argv() {}
    explicit argc_argv(const std::string &cmdline) 
    {
        parse(cmdline);
    }
};

} //graehl


#endif
