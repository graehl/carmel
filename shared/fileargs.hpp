#ifndef FILEARGS_HPP
#define FILEARGS_HPP

#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
//#include "fileheader.hpp"

typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;
typedef boost::shared_ptr<std::ifstream> InDiskfile;
typedef boost::shared_ptr<std::ofstream> OutDiskfile;

struct null_deleter {
    void operator()(void*) {}
};

static Infile default_in(&cin,null_deleter());
static Outfile default_log(&cerr,null_deleter());
static Outfile default_out(&cout,null_deleter());
static Infile default_in_none;
static Outfile default_out_none;

//using namespace boost;
//using namespace boost::program_options;
namespace po=boost::program_options;
using boost::shared_ptr;

//using namespace std;

namespace boost {    namespace program_options {
void validate(boost::any& v,
              const std::vector<std::string>& values,
              size_t* target_type, int)
{
    typedef size_t value_type;
    value_type size;
    double number;

    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    std::stringstream i(po::validators::get_single_string(values));
    if (!(i >> number))
        goto fail;
    char c;
    if (i.get(c)) {
        switch(c) {
        case 'g':case 'G':
            number *= (1024*1024*1024);
            break;

        case 'm':case 'M':
            number *= (1024*1024);
            break;

        case 'k':case 'K':
            number *=(1024);
            break;

        default:
            goto fail;
        }
    }
    if (i.get(c))
        goto fail;
    v=boost::any((value_type)number);
    if (number - (value_type)number > 1)
        throw std::runtime_error(std::string("Overflow - size too big for size_t: ").append(boost::lexical_cast<std::string>(number)));

    return;
fail:
    throw std::runtime_error(std::string("Expected nonnegative number followed by optional k,m, or g (2^10,2^20,2^30)suffix but got: ").append(i.str()));
}

/* Overload the 'validate' function for shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::istream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    if (s == "-") {
        boost::shared_ptr<std::istream> r(&std::cin, null_deleter());
    } else {
        boost::shared_ptr<std::ifstream> r(new std::ifstream(s.c_str()));
        if (!*r) {
            throw std::runtime_error(std::string("Could not open input file ").append(s));
        }
        boost::shared_ptr<std::istream> r2(r);
        v = boost::any(r2);
    }
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ostream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    if (s == "-") {
        boost::shared_ptr<std::ostream> w(&std::cout, null_deleter());
        v = boost::any(w);
    } else if ( s== "-2") {
        boost::shared_ptr<std::ostream> w(&std::cerr, null_deleter());
        v = boost::any(w);
    } else {
        boost::shared_ptr<std::ofstream> r(new std::ofstream(s.c_str()));
        if (!*r) {
            throw std::runtime_error(std::string("Could not create output file ").append(s));
        }
        boost::shared_ptr<std::ostream> w2(r);
        v = boost::any(w2);
    }
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ofstream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    boost::shared_ptr<std::ofstream> r(new std::ofstream(s.c_str()));
    if (!*r) {
        throw std::runtime_error(std::string("Could not create output file ").append(s));
    }
    v = boost::any(r);

}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ifstream>* target_type, int)
{
    //using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    po::validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = po::validators::get_single_string(values);

    boost::shared_ptr<std::ifstream> r(new std::ifstream(s.c_str()));
    if (!*r) {
        throw std::runtime_error(std::string("Could not open input file ").append(s));
    }
    v = boost::any(r);
}


}}


#endif
