#ifndef CHECKPOINT_ISTREAM_HPP
#define CHECKPOINT_ISTREAM_HPP

#include <iostream>
#include <fstream>

struct checkpoint_istream {
    std::fstream *f;
    typedef std::streampos pos;
    pos saved_pos;
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
    operator fstream & () { return *f; }
    operator bool () { return *f; } // can't double implicit convert
};

#endif
