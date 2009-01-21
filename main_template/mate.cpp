#define GRAEHL__SINGLE_MAIN

#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/debugprint.hpp>
#include <iostream>


#define MATE_VERSION "v1"
#define MATE_COMPILED " (compiled " MAKESTR_DATE ")"

char const *usage_str="\n"
    ""
    ;

using namespace std;

namespace graehl {

struct mate
{
    bool help;
    ostream_arg log_file;
    std::string cmdline_str;
    std::ostream *log_stream;
    std::auto_ptr<teebuf> teebufptr;
    std::auto_ptr<std::ostream> teestreamptr;
    
    template <class C>
    int carp(C const& c,const char*progname="mate") const
    {
        log() << "ERROR: " << c << "\n\n";
        log() << "Try '" << progname << " -h' for documentation\n";
        return 1;
    }
    
    void set_defaults() 
    {
    }

    void validate_parameters()
    {
        log_stream=log_file.get();
        if (!is_default_log(log_file)) // tee to cerr
        {
            teebufptr.reset(new teebuf(log_stream->rdbuf(),std::cerr.rdbuf()));
            teestreamptr.reset(log_stream=new std::ostream(teebufptr.get()));
        }

#ifdef DEBUG
        DBP::set_logstream(&log());
        DBP::set_loglevel(log_level);
#endif
        log() << "Command line: " << cmdline_str << "\n\n";        
    }

    void run()
    {
    }
    
    int main(int argc, char** argv)
    {
        const char *pr=argv[0];
        set_defaults();
        try {   
            if (!parse_args(argc,argv))
                return 1;
        } catch(...) {
            return 1;
        }
        try {
            validate_parameters();
            run();
        }
        catch(std::bad_alloc& e) {
            return carp("ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial parameters.",pr);
        }
        catch(std::exception& e) {
            return carp(e.what(),pr);
        }
        catch(const char * e) {
            return carp(e,pr);
        }
        catch(...) {
            return carp("FATAL Exception of unknown type!",pr);
        }
        return 0;
        
        return 0;
    }

    template <class OD>
    void add_options(OD &all) 
    {
        using boost::program_options::bool_switch;
        
        OD general("General options");
        general.add_options()
            ("help,h", bool_switch(&help),
             "show usage/documentation")
            ;

        OD cosmetic("Cosmetic options");
        cosmetic.add_options()
            ("log-file,l",defaulted_value(&log_file),
             "Send logs messages here (as well as to STDERR)")
            ;
        
        all.add(general).add(cosmetic);
        
        
    }    
    bool parse_args(int argc, char **argv) 
    {
        using namespace std;
        using namespace boost::program_options;
        typedef printable_options_description<std::ostream> OD;

        using namespace std;
        using namespace boost::program_options;
        typedef printable_options_description<std::ostream> OD;

        OD all("Allowed options");
        add_options(all);
        
        const char *progname=argv[0];
        try {
            cmdline_str=graehl::get_command_line(argc,argv,NULL);        
            //MAKESTRS(...,print_command_line(os,argc,argv,NULL));
            variables_map vm;            
            all.parse_options(argc,argv,vm);

            if (help) {
                cout << "\n" << progname << ' ' << "version " << get_version() << "\n\n";
                cout << usage_str << "\n";
                cout << all << "\n";
                return false;
            }
            log() << "### COMMAND LINE:\n" << cmdline_str << "\n\n";
            log() << "### CHOSEN OPTIONS:\n";
            all.print(log(),vm,OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);
        } catch (std::exception &e) {
            std::cerr << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
            throw;
        }
        return true;        
        
    }

    inline std::ostream &log() const {
        return log_stream ? *log_stream : std::cerr;
    }
    inline static std::string get_version() {
        return MATE_VERSION MATE_COMPILED;
    }
    inline void print(std::ostream &o) const {
        o << "mate-version={{{" << get_version() << "}}} mate-cmdline={{{"<<cmdline_str<<"}}}";
    }
    typedef mate self_type;

    friend inline std::ostream &operator<<(std::ostream &o,self_type const& s) 
    {
        s.print(o);
        return o;
    }
    
    
};

}

graehl::mate m;


int main(int argc, char** argv)
{
    return m.main(argc,argv);
}

    
