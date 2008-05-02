#ifndef GRAEHL__SHARED__OPTIONAL_PAIR_HPP
#define GRAEHL__SHARED__OPTIONAL_PAIR_HPP

#include <graehl/shared/stream_util.hpp>
#include <boost/config.hpp>
#include <functional>

namespace graehl {

// "5" <-> (5), "5/1" <-> (5,1) - second is optional (check has_second)
template <class T1,class T2=T1>
struct optional_pair //: public std::pair<T1,T2>
{
    BOOST_STATIC_CONSTANT(char,sep='/');
    
    typedef T1 first_type;
    typedef T2 second_type;
    first_type first;
    second_type second;
    bool has_second;
    typedef optional_pair<T1,T2> self_type;
    optional_pair() : first(), has_second(false) {}
    optional_pair(T1 const& f) : first(f), has_second(false) {}
    optional_pair(T1 const& f,T2 const& s) : first(f), second(s), has_second(true) {}
    optional_pair(self_type const& o) : first(o.first),second(o.second),has_second(o.has_second) {}
    self_type &operator=(self_type const& o) { first=o.first;second=o.second;has_second=o.has_second;return *this;}
    self_type &operator=(T1 const& f) 
    {
        first=f;
        has_second=false;
        return *this;
    }
    void set_second(T2 const& s) {
        second=s;
        has_second=true;
    }
    self_type& set(T1 const& f) {
        first=f;
        has_second=false;
        return *this;
    }
    self_type &set(T1 const& f,T2 const& s) {
        first=f;
        has_second=true;
        second=s;
        return *this;
    }
    template <class F>
    second_type get_second_default(F const& second_from_first) const 
    {
        if (has_second)
            return second;
        else
            return (second_type)second_from_first(first);
    }
    second_type get_second_default_constant(T2 const& c)  const
    {
        if (has_second)
            return second;
        else
            return c;
    }
    template <class O> void print(O&o) const
    {
        o<<first;
        if (has_second)
            o << sep << second;
    }
    template <class I> void read(I&i)
    {
        has_second=false;
        char c;
        if (i >> first) {
            if (i.get(c)) {
                if (c!=sep)
                    i.unget();
                else if (i >> second)
                    has_second=true;
            } else
                i.clear();            
        }
    }
    
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

}


#endif
