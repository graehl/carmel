#ifndef FILELINES_HPP
#define FILELINES_HPP

#include <iostream>
#include <fstream>
#include <limits>
#include "dynarray.h"
#include "myassert.h"
#include <string>
#include "fileargs.hpp"
#include <boost/lexical_cast.hpp>

typedef boost::shared_ptr<std::ifstream> InSeekFile;

struct FileLines
{
    static const char linesep='\n';
    InDiskfile file;
    typedef std::streampos pos;
    DynamicArray<pos> line_begins; // line i ranges from [line_begins[i],line_begins[i+1])
    FileLines(std::string filename) { load(filename); }
    FileLines(InDiskfile _file) : file(_file) { if (file) index(); }
    void index() {
        file->seekg(0);
        line_begins.clear();
        line_begins.push_back(file->tellg());
        while (!file->eof()) {
            file->ignore(std::numeric_limits<std::streamsize>::max(),linesep); //FIXME: does setting max unsigned really work here?  not if you have very very long lines ;)  but those don't fit into memory.
            line_begins.push_back(file->tellg());
        }
        pos eof=file->tellg();
        if (eof > line_begins.back())
            line_begins.push_back(eof);
    }
    void load(std::string filename) {
        file.reset(new std::ifstream);
        file->open(filename.c_str(),std::ios::in | std::ios::binary);
        index();
    }
    std::string operator [](unsigned i) {
        if (!file) {
            return boost::lexical_cast<std::string>(i);
        }
        Assert(i>0 && i+1 < line_begins.size());
        pos start=line_begins[i];
        pos end=line_begins[i+1];
        unsigned len=end-start;
        AutoArray<char> buf(len); // FIXME: could just return a shared_ptr or use string = getline(blah,sep) ... or return expression object that can convert to string or be printed
        file->seekg(start);
        file->read(buf.begin(),len);
        return std::string(buf.begin(),len);
    }
    bool exists() const {
        return file;
    }
    unsigned size() const {
        Assert(exists() && line_begins.size()>0);
        return line_begins.size()-1;
    }
};


#endif
