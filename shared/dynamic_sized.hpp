#ifndef GRAEHL__SHARED__DYNAMIC_SIZED_HPP
#define GRAEHL__SHARED__DYNAMIC_SIZED_HPP

#include <stdexcept>
#include <cassert>
#include <new>
#include <cstring>
#include <graehl/shared/hash_functions.hpp>

namespace graehl {

//TODO: template all on word type (allow char), and pascal allow different length type?

// use as a base class for unsigned-aligned pod array types (instead of void *, char *).  for documentation.  also gives the alignment we almost always want (4 bytes)
struct dynamic_sized
{
    unsigned front;


    unsigned const* begin() const
    {
        return &front;
    }

    unsigned * begin()
    {
        return &front;
    }

    dynamic_sized() {}
 private:
    dynamic_sized(dynamic_sized const& o)
    {
        assert(0);
        throw std::runtime_error("can't copy construct dynamically sized objects");
    }
};


/// a pascal string has first a size (front) and immediately adjacent in memory that many words.  in this case, the words are unsigned (32bit) rather than chars
struct pascal_words_string : public dynamic_sized
{
    typedef unsigned const* quad_p;

    bool is_null()
    {
        return front==(unsigned)-1;
    }

    void set_null()
    {
        front=(unsigned)-1;
    }

    bool equal(quad_p p,unsigned len) const // may be called even if is_null()
    {
        return front==len && 0==std::memcmp(begin()+1,p,len*sizeof(unsigned));
    }

    void set(quad_p p,unsigned len)
    {
        front=len;
        std::memcpy(begin()+1,p,len*sizeof(unsigned));
    }

};

template <class Fixed,class Dynamic=pascal_words_string>
struct dynamic_pair
{
    Fixed fixed;
    Dynamic dynamic;
};

inline unsigned n_unsigned(unsigned bytes)
{
    return (bytes+sizeof(unsigned)-1)/sizeof(unsigned);
}


/// each dynamic_sized object must be of bounded size.  further, the objects aren't initialized (i.e. size header isn't even set to 0)
//TODO: allocator ratehr than ::new
template <class V>
struct dynamic_sized_array
{
    typedef V value_type; // note: actual size is greater than sizeof(V)
    typedef dynamic_sized_array<V> self_type;

    dynamic_sized_array(unsigned n_objects,unsigned max_object_words)
        : size(n_objects)
        , max_object_words(max_object_words)
        , words((unsigned*)::operator new(sizeof(unsigned)*max_object_words*n_objects))
    {}

    ~dynamic_sized_array()
    {
        ::operator delete(words);
    }

    value_type &operator[](unsigned i)
    {
        return (value_type&)words[i*max_object_words];
    }
    value_type const&operator[](unsigned i) const
    {
        return const_cast<self_type&>(*this)[i];
    }

    unsigned *begin_words() const
    {
        return words;
    }

    unsigned *end_words() const
    {
        return words+n_words();
    }

    unsigned n_words() const
    {
        return size*max_object_words;
    }

    void set_words(unsigned to=0)
    {
        for (unsigned i=0,N=n_words();i<N;++i)
            words[i]=to;
    }

    void set_first_words(unsigned to=0)
    {
        for (unsigned i=0,N=n_words();i<N;i+=max_object_words)
            words[i]=to;
    }

    // these aren't automatically called
    void construct()
    {
        for (unsigned i=0;i<size;++i)
            new(&(*this)[i]) value_type();
    }
    template <class T>
    void construct(T const& t)
    {
        for (unsigned i=0;i<size;++i)
            new(&(*this)[i]) value_type(t);
    }
    void destroy()
    {
        for (unsigned i=0;i<size;++i)
            (*this)[i].~value_type();
    }

    unsigned size,max_object_words;
 private:
    unsigned *words;
};


}

#endif
