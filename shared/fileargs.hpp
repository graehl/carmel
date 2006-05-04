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

#ifndef GRAEHL__DEFAULT_IN_P
#define GRAEHL__DEFAULT_IN_P &std::cin
#endif

#ifndef GRAEHL__DEFAULT_OUT_P
#define GRAEHL__DEFAULT_OUT_P &std::cout
#endif

#ifndef GRAEHL__DEFAULT_LOG_P
#define GRAEHL__DEFAULT_LOG_P &std::cerr
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
    explicit file_arg(std::string const& s,
             bool read=stream_traits<Stream>::read,
             bool file_only=stream_traits<Stream>::file_only)
    { set(s,read,file_only); }

    void set(Stream *s) 
    {
        pointer().reset(s,null_deleter());
        name="";
    }
    // warning: if you call with incompatible filestream type ... crash!
    template <class filestream>
    void set(std::string const& filename,char const* fail_msg="Couldn't open file ")
    {
//        boost::shared_ptr<filestream>
        filestream *f=new filestream(filename.c_str());
        if (!*f) {
            delete f;
            throw std::runtime_error(std::string(fail_msg).append(filename));
        }
        
        pointer().reset((Stream*)f);
        name=filename;
    }
    
    enum { ALLOW_NULL=1,NO_NULL=0 };

    // warning: if you specify the wrong values for read and file_only, you could assign the wrong type of pointer and crash!
    void set(std::string const& s,
             bool null_allowed=ALLOW_NULL,
             bool read=stream_traits<Stream>::read,
             bool file_only=stream_traits<Stream>::file_only)
    {
        if (s.empty())
            throw std::runtime_error("Tried to make file_arg stream from empty filename");
        if (!file_only && s == "-")
            set(read ? (Stream*)GRAEHL__DEFAULT_IN_P : (Stream*)GRAEHL__DEFAULT_OUT_P);
        else if (!file_only && !read && s== "-2")
            set((Stream *)GRAEHL__DEFAULT_LOG_P);
        else if (null_allowed && s == "-0")
            set(NULL);
        else if (match_end(s.begin(),s.end(),gz_ext,gz_ext+sizeof(gz_ext)-1)) {
            if (read)
                set<igzstream>(s,"Couldn't open compressed input file ");
            else
                set<ogzstream>(s,"Couldn't create compressed output file ");
            } else {
                if (read)
                    set<std::ifstream>(s,"Couldn't open compressed input file ");
                else
                    set<std::ofstream>(s,"Couldn't create compressed output file ");
            }
            name=s;
        }
    
    file_arg(Stream &s,std::string const& name) :
        pointer_type(*s,null_deleter()),name(name) {}
    
    template <class Stream2>
    file_arg(file_arg<Stream2> const& o) :
        pointer_type(o.pointer()),name(o.name) {}    

    bool is_none() const
    { return !pointer(); }
    
    void set_none() const
    { pointer().reset(0);name=""; }
    
    bool is_default_in() const {
        return pointer().get() == GRAEHL__DEFAULT_IN_P; }
    
    bool is_default_out() const {
        return pointer().get() == GRAEHL__DEFAULT_OUT_P; }
    
    bool is_default_log() const {
        return pointer().get() == GRAEHL__DEFAULT_LOG_P; }
    
    pointer_type &pointer() { return *this; }
    pointer_type const& pointer() const { return *this; }

    bool valid() const
    {
        return !is_none() && stream();
    }
    
    Stream &stream() const
    {
        return *pointer();
    }
    operator Stream &() const
    {
        return stream();
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

typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

static Infile default_in(GRAEHL__DEFAULT_IN_P,null_deleter());
static Outfile default_log(GRAEHL__DEFAULT_LOG_P,null_deleter());
static Outfile default_out(GRAEHL__DEFAULT_OUT_P,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;
static InDiskfile default_in_disk_none;
static OutDiskfile default_out_disk_none;

inline bool is_default_in(const Infile &i) {
    return i.get() == GRAEHL__DEFAULT_IN_P; }

inline bool is_default_out(const Outfile &o) {
    return o.get() == GRAEHL__DEFAULT_OUT_P; }

inline bool is_default_log(const Outfile &o) {
    return o.get() == GRAEHL__DEFAULT_LOG_P; }

inline bool is_none(const Infile &i) 
{ return i.get()==NULL; }

inline bool is_none(const Outfile &o) 
{ return o.get()==NULL; }

struct tee_file 
{
    tee_file() : log_stream(&std::cerr) {}
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
