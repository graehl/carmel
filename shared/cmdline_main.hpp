#ifndef GRAEHL_SHARED__CMDLINE_MAIN_HPP
#define GRAEHL_SHARED__CMDLINE_MAIN_HPP

#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/debugprint.hpp>
#include <iostream>

#define INT_MAIN(main_class) main_class m;                  \
    int main(int argc,char **argv) { return m.run(argc,argv); }

#define GRAEHL_MAIN_COMPILED " (compiled " MAKESTR_DATE ")"

namespace graehl {

struct main
{
    typedef main self_type;

    friend inline std::ostream &operator<<(std::ostream &o,self_type const& s)
    {
        s.print(o);
        return o;
    }

    virtual void print(std::ostream &o) const {
        o << appname<<"-version={{{" << get_version() << "}}} "<<appname<<"-cmdline={{{"<<cmdline_str<<"}}}";
    }

    std::string get_name() const
    {
        return appname+"-"+get_version();
    }

    std::string get_version() const
    {
        return version+compiled;
    }


    int debug_lvl;
    bool help;
    ostream_arg log_file,out_file;
    std::string cmdname,cmdline_str;
    std::ostream *log_stream;
    std::auto_ptr<teebuf> teebufptr;
    std::auto_ptr<std::ostream> teestreamptr;

    std::string appname,version,compiled,usage;
    //FIXME: segfault if version is a char const* global in subclass xtor, why?
    main(std::string const& name="main",std::string const& usage="usage undocumented\n",std::string const& version="v1",std::string const& compiled=GRAEHL_MAIN_COMPILED)  : appname(name),version(version),compiled(compiled),usage(usage),general("General options"),cosmetic("Cosmetic options"),all_options("Allowed options"),options_added(false)
    {
    }

    typedef printable_options_description<std::ostream> OD;
    OD general,cosmetic,all_options;
    bool options_added;

    virtual void run()
    {
        out() << cmdname << ' ' << version << ' ' << compiled << std::endl;
    }

    virtual void set_defaults()
    {
        set_defaults_base();
    }

    virtual void validate_parameters()
    {
        validate_parameters_base();
    }

    // this should only be called once.  (called after set_defaults)
    virtual void add_options(OD &all)
    {
        if (options_added)
            return;
        add_options_base(all,true);
        add_options_extra(all);
        finish_options(all);
    }

    virtual void finish_options(OD &all)
    {
        all.add(general).add(cosmetic);
        options_added=true;
    }

    virtual void add_options_extra(OD &all) {}

    virtual void log_invocation()
    {
        log_invocation_base();
    }


    void set_defaults_base()
    {
        out_file=stdout_arg();
        log_file=stderr_arg();
    }

    inline std::ostream &log() const {
        return *log_stream;
    }

    inline std::ostream &out() const {
        return *out_file;
    }

    void log_invocation_base()
    {
        log() << "### COMMAND LINE:\n" << cmdline_str << "\n\n";
        log() << "### CHOSEN OPTIONS:\n";
        all_options.print(log(),vm,OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);
    }

    void validate_parameters_base()
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

    }

    void add_options_base(OD &all,bool add_out_file=true)
    {
        using boost::program_options::bool_switch;

        general.add_options()
            ("help,h", bool_switch(&help),
             "show usage/documentation")
            ;


        if (add_out_file)
            cosmetic.add_options()
                ("out-file,o",defaulted_value(&out_file),
                 "Output here (instead of STDOUT)");

        cosmetic.add_options()
            ("log-file,l",defaulted_value(&log_file),
             "Send logs messages here (as well as to STDERR) - FIXME: must end in .gz")
            ("debug-level,d",defaulted_value(&debug_lvl),
             "Debugging output level (0 = off, 0xFFFF = max)")
            ;

    }

    boost::program_options::variables_map vm;

    bool parse_args(int argc, char **argv)
    {
        using namespace std;
        using namespace boost::program_options;

        add_options(all_options);

        try {
            cmdline_str=graehl::get_command_line(argc,argv,NULL);
            all_options.parse_options(argc,argv,vm);

            if (help) {
                cout << "\n" << get_name() << "\n\n";
                cout << general_options_desc();
                cout << usage << "\n";
                cout << all_options << "\n";
                return false;
            }
        } catch (std::exception &e) {
            std::cerr << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
            throw;
        }
        return true;
    }


    //FIXME: defaults cannot change after first parse_args
    int run(int argc, char** argv)
    {
        cmdname=argv[0];
        set_defaults();
        try {
            if (!parse_args(argc,argv))
                return 1;
            validate_parameters();
            log_invocation();
            run();
        }
        catch(std::bad_alloc& e) {
            return carp("ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial parameters.");
        }
        catch(std::exception& e) {
            return carp(e.what());
        }
        catch(const char * e) {
            return carp(e);
        }
        catch(...) {
            return carp("FATAL Exception of unknown type!");
        }
        return 0;

    }

    template <class C>
    int carp(C const& c) const
    {
        log() << "\nERROR: " << c << "\n\n";
        log() << "Try '" << cmdname << " -h' for documentation\n";
        return 1;
    }

    virtual ~main() {}
};



}



#endif
