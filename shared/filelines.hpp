#ifndef FILELINES_HPP
#define FILELINES_HPP

#include <iostream>
#include <fstream>
#include <limits>
#include "dynarray.h"
#include "myassert.h"

struct FileLines
{
    const char linesep='\n';
    std::fstream file;
    typedef std::streampos pos;
    DynamicArray<pos> line_begins; // line i ranges from [line_begins[i],line_begins[i+1])
    FileLines(string filename) { load(filename); }
    load(string filename) {
        file.open(filename.c_str(),std::ios::in | std::ios::binary);
        line_begins.clear();
        line_begins.push_back(file.tellg());
        while (!file.eof()) {
            file.ignore(std::numeric_limits<std::streamsize>::max(),linesep); //FIXME: does setting max unsigned really work here?  not if you have very very long lines ;)  but those don't fit into memory.
            line_begins.push_back(file.tellg());
        }
        pos eof=file.tellg();
        if (eof > line_begins.back())
            line_begins.push_back(eof);
    }
    string operator [](unsigned i) {
        Assert(i>0 && i+1 < line_begins.size());
        pos start=line_begins[i];
        pos end=line_begins[i+1];

    }
};


#endif
