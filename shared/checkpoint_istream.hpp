// used by swapbatch.hpp (originally planned on providing unbounded since-last-checkpoint putback buffer to obviate seek())
#ifndef CHECKPOINT_ISTREAM_HPP
#define CHECKPOINT_ISTREAM_HPP

#include <iostream>
#include <fstream>
#include <graehl/shared/byref.hpp>

namespace graehl {

template <class C>
struct checkpoint_istream_control;

template <>
struct checkpoint_istream_control<std::fstream> {
    typedef std::streampos pos;
    pos saved_pos;
    checkpoint_istream_control() : saved_pos(0) {}
    template <class T>
    void revert(T &t) {
        t.seekg(saved_pos);
    }
    template <class T>
    void commit(T &t) {
        saved_pos=t.tellg();
    }
    typedef boost::reference_wrapper<std::fstream> istream_wrapper;
};
/*
struct checkpoint_istream {
    std::fstream *f;
    typedef std::fstream::char_type                     char_type;
    typedef char_traits<char_type> traits;
    typedef traits::int_type int_type;
    typedef traits::pos_type pos_type;
    typedef traits::off_type off_type;
    typedef traits                    traits_type;
    typedef checkpoint_istream __istream_type;
    pos_type saved_pos;
    void init(std::fstream *f_) {
        f=f_;
        commit();
    }
    checkpoint_istream() : f(NULL) {}
    explicit checkpoint_istream(std::istream &i) {
        init(&dynamic_cast<std::fstream &>(i));
    }
    void revert() {
        f->seekg(saved_pos);
    }
    void commit() {
        saved_pos=f->tellg();
    }
    template <class charT,class Traits>
    operator std::basic_istream<charT,Traits>&() { return *f; }
    operator std::fstream & () { return *f; }
    operator bool () const { return f->good(); } // can't double implicit convert
    __istream_type& unget() { f->unget(); return *this; }
    __istream_type&
    get(char_type& __c) {
        f->get(__c); return *this;
    }
      __istream_type&
      ignore(streamsize __n = 1, int_type __delim = traits_type::eof()) {
          f->ignore(__n,__delim); return *this;
      }
// ... and many more ;(  use boost IOStreams library when it's available
    bool good() const { return f->good(); }
    void setstate(std::ios::iostate state) { f->setstate(state); }
};
*/

/* won't work: can't copy stream
struct checkpoint_istream : public std::fstream {
    checkpoint_istream(std::istream &i) : std::fstream(dynamic_cast<std::fstream &>(i)) {
    }
    typedef std::streampos pos;
    pos saved_pos;
    void revert() {
        seekg(saved_pos);
    }
    void commit() {
        saved_pos=tellg();
    }
};
*/
}
#endif
