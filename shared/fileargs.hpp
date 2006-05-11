// given a filename, creates a (reference counted) input/output file/stream object, with "-" = STDIN/STDOUT, and ".gz" appropriately (de)compressed using gzstream.h - also, parameter parsing for Boost (command-line) Options library 
#ifndef GRAEHL__SHARED__FILEARGS_HPP
#define GRAEHL__SHARED__FILEARGS_HPP

#include <graehl/shared/teestream.hpp>
#include <graehl/shared/gzstream.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/string_match.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>

#ifndef GRAEHL__DEFAULT_IN
#define GRAEHL__DEFAULT_IN std::cin
#endif

#ifndef GRAEHL__DEFAULT_OUT
#define GRAEHL__DEFAULT_OUT std::cout
#endif

#ifndef GRAEHL__DEFAULT_LOG
#define GRAEHL__DEFAULT_LOG std::cerr
#endif


namespace graehl {

template <class Stream>
struct stream_traits 
{
};

    
template<>
struct stream_traits<std::ifstream>
{
    BOOST_STATIC_CONSTANT(bool,file_only=true);
    BOOST_STATIC_CONSTANT(bool,read=true);
};

template<>
struct stream_traits<std::ofstream>
{
    BOOST_STATIC_CONSTANT(bool,file_only=true);
    BOOST_STATIC_CONSTANT(bool,read=false);
};
    
template<>
struct stream_traits<std::ostream>
{
    BOOST_STATIC_CONSTANT(bool,file_only=false);
    BOOST_STATIC_CONSTANT(bool,read=false);
};

template<>
struct stream_traits<std::istream>
{
    BOOST_STATIC_CONSTANT(bool,file_only=false);
    BOOST_STATIC_CONSTANT(bool,read=true);
};

static const char gz_ext[]=".gz";

struct null_deleter {
    void operator()(void*) const {}
};

template <class Stream>
struct file_arg : public boost::shared_ptr<Stream>
{
    std::string name;
    typedef boost::shared_ptr<Stream> pointer_type;
    file_arg() {}
    explicit file_arg(std::string const& s,bool null_allowed=ALLOW_NULL)
    { set(s,null_allowed); }
    void throw_fail(std::string const& filename,std::string const& msg="")
    {
        name=filename;
        throw std::runtime_error("FAILED("+filename+"): "+msg);
    }

    enum { DELETE=1,NO_DELETE=0 };
    
    void set(Stream &s,std::string const& filename="",bool destroy=NO_DELETE,std::string const& fail_msg="invalid stream") 
    {
        if (!s)
            throw_fail(filename,fail_msg);
        if (destroy)
            pointer().reset(&s);
        else
            pointer().reset(&s,null_deleter());
        name=filename;
    }
    
    template <class filestream>
    void set_checked(filestream &fs,std::string const& filename="",bool destroy=NO_DELETE,std::string const& fail_msg="invalid stream")
    {
        try {
            set(dynamic_cast<Stream &>(fs),filename,destroy,fail_msg);
        } catch (std::bad_cast &e) {
            throw_fail(filename," was not of the right stream type");
        }
    }
    // warning: if you call with incompatible filestream type ... crash!
    template <class filestream>
    void set_new(std::string const& filename,std::string const& fail_msg="Couldn't open file")
    {
        std::auto_ptr<filestream> f(new filestream(filename.c_str()));
        set_checked(*f,filename,DELETE,fail_msg);
        f.release(); // w/o delete
    }
    
    enum { ALLOW_NULL=1,NO_NULL=0 };

    void set_gzfile(std::string const&s)
    {
        const bool read=stream_traits<Stream>::read;
        std::string fail_msg="Couldn't open compressed input file";
        try {
            if (read) {
                set_new<igzstream>(s,fail_msg);
            } else {
                fail_msg="Couldn't create compressed output file";
                set_new<ogzstream>(s,fail_msg);
            }
        } catch (std::exception &e) {
            fail_msg.append(" - exception: ").append(e.what());
            throw_fail(s,fail_msg);
        }
    }
    
    // warning: if you specify the wrong values for read and file_only, you could assign the wrong type of pointer and crash!
    void set(std::string const& s,
             bool null_allowed=ALLOW_NULL)
    {
        const bool read=stream_traits<Stream>::read;
        const bool file_only=stream_traits<Stream>::file_only;
        if (s.empty()) {
            throw_fail("<EMPTY FILENAME>","Can't open an empty filename.  Use \"-0\" if you really mean no file");            
        } if (!file_only && s == "-") {
            if (read)
                set_checked(GRAEHL__DEFAULT_IN,s);
            else
                set_checked(GRAEHL__DEFAULT_OUT,s);
        } else if (!file_only && !read && s== "-2")
            set_checked(GRAEHL__DEFAULT_LOG,s);
        else if (null_allowed && s == "-0")
            set_none();
        else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
            set_gzfile(s);
        } else {
            if (read)
                set_new<std::ifstream>(s,"Couldn't open input file");
            else
                set_new<std::ofstream>(s,"Couldn't create output file");
        }
    }
    
    explicit file_arg(Stream &s,std::string const& name) :
        pointer_type(*s,null_deleter()),name(name) {}
    
    template <class Stream2>
    file_arg(file_arg<Stream2> const& o) :
        pointer_type(o.pointer()),name(o.name) {}    

    bool is_none() const
    { return !pointer(); }
    
    void set_none()
    { pointer().reset();name="-0"; }
    
    bool is_default_in() const {
        return pointer().get() == &GRAEHL__DEFAULT_IN; }
    
    bool is_default_out() const {
        return pointer().get() == &GRAEHL__DEFAULT_OUT; }
    
    bool is_default_log() const {
        return pointer().get() == &GRAEHL__DEFAULT_LOG; }
    
    pointer_type &pointer() { return *this; }
    pointer_type const& pointer() const { return *this; }

    bool valid() const
    {
        return !is_none() && stream();
    }
    friend
    bool valid(file_arg<Stream> const& f)
    {
        return f.valid();
    }
    
    Stream &stream() const
    {
        return *pointer();
    }
    template<class O>
    friend O& operator <<(O &o,file_arg<Stream> const& me)
    {
        o << me.name;
        return o;
    }
    template<class C,class T>
    friend std::basic_istream<C,T>& operator >>(std::basic_istream<C,T> &i,file_arg<Stream> & me)
    {
        std::string name;
        i>>name;
        me.set(name);
        return i;
    }
};

typedef file_arg<std::istream> istream_arg;
typedef file_arg<std::ostream> ostream_arg;
typedef file_arg<std::ifstream> ifstream_arg;
typedef file_arg<std::ofstream> ofstream_arg;

istream_arg stdin_arg()
{
    return istream_arg("-");
}

ostream_arg stdout_arg()
{
    return ostream_arg("-");
}

ostream_arg stderr_arg()
{
    return ostream_arg("-2");
}


typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

static Infile default_in(&GRAEHL__DEFAULT_IN,null_deleter());
static Outfile default_log(&GRAEHL__DEFAULT_LOG,null_deleter());
static Outfile default_out(&GRAEHL__DEFAULT_OUT,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(const Infile &i) {
    return i.get() == &GRAEHL__DEFAULT_IN; }

inline bool is_default_out(const Outfile &o) {
    return o.get() == &GRAEHL__DEFAULT_OUT; }

inline bool is_default_log(const Outfile &o) {
    return o.get() == &GRAEHL__DEFAULT_LOG; }

inline bool is_none(const Infile &i) 
{ return i.get()==NULL; }

inline bool is_none(const Outfile &o) 
{ return o.get()==NULL; }

struct tee_file 
{
    tee_file() {
        reset_no_tee();
    }
    
    void reset_no_tee(std::ostream &o=std::cerr) 
    {
        teestreamptr.reset();
        teebufptr.reset();
        log_stream=&o;
    }
    
    /// must call before you get any tee behavior (without, will go to default log = cerr)!
    void set(std::ostream &other_output) 
    {
        if (file) {
            teebufptr.reset(
                new graehl::teebuf(file->rdbuf(),other_output.rdbuf()));
            teestreamptr.reset(
                log_stream=new std::ostream(teebufptr.get()));
        } else {
            log_stream=&other_output;
        }
    }
    ostream_arg file; // can set this directly, then call init.  if unset, then don't tee.
    
    std::ostream &stream() const
    { return *log_stream; }
    operator std::ostream &() const
    { return stream(); }
    
 private:
    std::ostream *log_stream;
    boost::shared_ptr<graehl::teebuf> teebufptr;
    boost::shared_ptr<std::ostream> teestreamptr;           
};

template <class Stream>
inline bool valid(boost::shared_ptr<Stream> const& pfile)
{
    return pfile && *pfile;
}

template <class C>
inline bool throw_unless_valid(C const& pfile, std::string const& name="file") 
{
    if (!valid(pfile))
        throw std::runtime_error(name+" not valid");
}

template <class C>
inline bool throw_unless_valid_optional(C const& pfile, std::string const& name="file") 
{
    if (pfile && !valid(pfile))
        throw std::runtime_error(name+" not valid");
}

inline Infile infile(const std::string &s) 
{    
    return istream_arg(s);
}

inline InDiskfile indiskfile(const std::string &s)
{
    return ifstream_arg(s);
}

namespace fs = boost::filesystem;

inline fs::path full_path(const std::string &relative) 
{
    return fs::system_complete( fs::path( relative, fs::native ) );
}

inline bool directory_exists(const fs::path &possible_dir)
{
    return fs::exists(possible_dir) && fs::is_directory(possible_dir);
}

// works on .gz files!
inline size_t count_newlines(const std::string &filename) 
{
    Infile i=infile(filename);
    char c;
    size_t n_newlines=0;
    while (i->get(c)) {
        if (c=='\n')
            ++n_newlines;
    }
    return n_newlines;    
}


// return the absolute filename that would result from "cp source dest" (and write to *dest_exists whether dest exists) - throws error if source is the same as dest
inline std::string output_name_like_cp(const std::string &source,const std::string &dest,bool *dest_exists=NULL) 
{
    fs::path full_dest=full_path(dest);
    fs::path full_source=full_path(source);
    
    if (directory_exists(full_dest))
        full_dest /= full_source.leaf();
    if (dest_exists && fs::exists(full_dest))
        *dest_exists=1;
        

    full_dest.normalize();
    full_source.normalize();
    
    if (fs::equivalent(full_dest,full_source))
        throw std::runtime_error("Destination file is same as source!");
    
    return full_dest.native_file_string();
}

inline Outfile outfile(const std::string &s) 
{
    return ostream_arg(s);
}

inline OutDiskfile outdiskfile(const std::string &s)
{
    return ofstream_arg(s);
}

inline std::string const& get_single_arg(boost::any& v,std::vector<std::string> const& values) 
{
    boost::program_options::validators::check_first_occurrence(v);
    return boost::program_options::validators::get_single_string(values);
}

} //graehl

namespace boost {    namespace program_options {
inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     size_t* target_type, int)
{
    typedef size_t value_type;
    using namespace graehl;

    std::istringstream i(boost::program_options::validators::get_single_string(values));
    v=boost::any(graehl::parse_size<value_type>(i));
    char c;
    if (i >> c)
        throw std::runtime_error(std::string("Succesfully read a nonnegative size, but read extra characters after."));
    return;
}

#ifdef GRAEHL__VALIDATE_INFILE
/* Overload the 'validate' function for boost::shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     boost::shared_ptr<std::istream>* target_type, int)
{
    v=boost::any(graehl::infile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     boost::shared_ptr<std::ostream>* target_type, int)
{
    v=boost::any(graehl::outfile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     boost::shared_ptr<std::ofstream>* target_type, int)
{
    v=boost::any(graehl::outdiskfile(graehl::get_single_arg(v,values)));
}

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     boost::shared_ptr<std::ifstream>* target_type, int)
{
    v=boost::any(graehl::indiskfile(graehl::get_single_arg(v,values)));
}
#endif

}} // boost::program_options


#endif
