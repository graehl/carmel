// store in memory only the file positions necessary to quickly extract line #N from a large file
#ifndef FILELINES_HPP
#define FILELINES_HPP

//TODO: * parsing of id=N rules files (instead of assuming line # = N)

#include <iostream>
#include <fstream>
#include <limits>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/myassert.h>
#include <string>
#include <graehl/shared/fileargs.hpp>
#include <boost/lexical_cast.hpp>

#ifdef WINDOWS
# undef max
#endif 

namespace graehl {
typedef boost::shared_ptr<std::ifstream> InSeekFile;
struct FileLines
{
    static const char linesep='\n';
    InDiskfile file;
    typedef std::streampos pos;
    dynamic_array<pos> line_begins; // line i ranges from [line_begins[i],line_begins[i+1])
    FileLines(std::string filename) { load(filename); }
    FileLines(InDiskfile _file) : file(_file) { if (file) index(); }
    void index() {
        file->seekg(0);
        line_begins.clear();
        line_begins.push_back(file->tellg());
        while (!file->eof()) {
            file->ignore(std::numeric_limits<std::streamsize>::max(),linesep); //FIXME: does setting max unsigned really work here?  not if you have very very long lines ;)  but those don't fit into memory.
            pos end=file->tellg();
            if (end > line_begins.back())
                line_begins.push_back(end);
        }
//        should never be needed!
        Assert(file->tellg() == line_begins.back());
/*        pos eof=file->tellg();
        if (eof > line_begins.back()) { // only necessary to handle lack of trailing newline in a friendly way
            line_begins.push_back(eof);
            }*/

        file->clear(); // don't want EOF flag stopping reads.
        line_begins.compact();
    }
    void load(std::string filename) {
        file.reset(new std::ifstream);
        file->open(filename.c_str(),std::ios::in | std::ios::binary);
        index();
    }
    // 0-indexed, of course
    std::string getline(unsigned i,bool chop_newline=true) {
        if (!file) {
            return boost::lexical_cast<std::string>(i);
        }
        Assert(i>=0 && i+1 < line_begins.size());
        pos start=line_begins[i];
        pos end=line_begins[i+1];
        unsigned len=end-start;
        if (chop_newline && len > 0)
            --len; // don't want to include newline char
        auto_array<char> buf(len); // FIXME: could just return a shared_ptr or use string = getline(blah,sep) ... or return expression object that can convert to string or be printed
        file->seekg(start);
        file->read(buf.begin(),len);
        return std::string(buf.begin(),len);
    }
    std::string operator [](unsigned i) {
        return getline(i);
    }
    bool exists() const {
        return file;
    }
    unsigned size() const {
        Assert(exists() && line_begins.size()>0);
        return line_begins.size()-1;
    }
};

}

#endif
