#ifndef ARRAY_STREAM_HPP
#define ARRAY_STREAM_HPP

// fixed size user-supplied memory stream buffer (overflow = error on write, eof on read)
// NOTE: end() returns the current write position (i.e. should only be used to retrieve written data)

#include <streambuf>
#include <ostream>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <cstddef>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/stream_util.hpp>

namespace graehl {


// doesn't quite work like a streambuf: end of readable area isn't increased when you write, without explicit reset_read(size())
template<class cT, class Traits = std::char_traits<cT> >
class basic_array_streambuf : public std::basic_streambuf<cT, Traits>
{
 public:
    typedef Traits traits;
    typedef std::basic_streambuf<cT, traits> base;
    typedef basic_array_streambuf<cT, traits> self;
    typedef typename traits::char_type char_type;
    typedef typename base::pos_type pos_type;
    typedef typename base::off_type off_type;
    typedef typename base::int_type int_type;
    typedef std::streamsize size_type;

    typedef cT value_type;
    typedef value_type *iterator;     //NOTE: you can count on iterator being a pointer

    // shows what's been written
    template <class O>
    void print(O *o) const 
    {
        o << "array_streambuf[" << capacity() << "]=[";
        for (typename self::iterator i=begin(),e=end();i!=e;++i)
            o.put(*i);
        o << "]";
    }
    TO_OSTREAM_PRINT

    explicit
    basic_array_streambuf(const char_type * p = 0, size_type sz = 0)
    {
        set_array(p,sz);
    }
    inline void set_array(const char_type * p, size_type sz = 0)
    {
        buf = const_cast<char_type *>(p);
        bufend = buf+sz;
        setg(buf, buf, bufend);
        setp(buf, bufend);
//        setp(0,0); //fixme: should probably set this for const - on your honor, don't write to const buffers
    }
    inline void reset_read(size_type sz)
    {
        setg(buf,buf,buf+sz);
    }
    // default: read what was written.
    inline void reset_read()
    {
        setg(buf,buf,end());
    }

    void reset() 
    {
        set_gpos(0);
        set_ppos(0);
    }
    // doesn't adjust readable end (read_size)
    void set_gpos(pos_type pos) 
    {
        setg(buf, buf+pos, egptr());
    }
    void set_end(iterator end_)
    {
        setp(end_,bufend);
    }   
    void set_ppos(pos_type pos) 
    {
        setp(buf+pos, bufend);
    }
    // doesn't adjust current read pos - danger if past end
    inline void set_read_size(size_type sz)
    {
        setg(buf,gptr(),buf+sz);
    }
    inline void set_write_size(size_type sz)
    {
        set_ppos(sz);
    }
    
// set both read and write size
    inline void set_size(size_type sz)
    {
        set_read_size(sz);
        set_write_size(sz);
    }
    
    inline std::ptrdiff_t bufsize() const
    {
        return capacity();
    }
    inline std::ptrdiff_t capacity() const
    {
        return bufend-buf;
    }
    using base::in_avail;
    // default base::in_avail()
    inline std::ptrdiff_t out_avail() 
    {
        return bufend - pptr();
    }
    // default base::overflow(c) - always fails
    int_type overflow(int_type c) {
        if (out_avail()==0 && !traits::eq_int_type(c, traits::eof()))
            return traits::eof();  // FAIL
        return traits::not_eof(c);
    }

    /*
    //NOTE: maybe the default sputn would have been just as fast given we've set a buffer ... not sure
    // below defined for speedup (no repeated virtual get/put one char at a time)
    virtual std::streamsize xsputn(const char_type * from, std::streamsize sz)
    {
        if(out_avail()<sz)
            return 0;
        traits::copy(pptr(), from, sz);
        DBP2(sz,std::string(from,sz));
        pbump(sz);        
        return sz;        
    }
    //ACTUALLY: will never get called if you just use single-character get()
    virtual std::streamsize xsgetn(char_type * to, std::streamsize sz)
    {
        const std::streamsize extracted = std::min(in_avail(), sz);
        traits::copy(to, gptr(), extracted);
        DBP3(sz,extracted,std::string(to,extracted));
        gbump(extracted);
        return extracted;
    }
    */
    
    // not really necessary to support but nice ... for seekp, seekg
    virtual pos_type seekoff(off_type offset,
                             std::ios_base::seekdir dir,
                             std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        pos_type pos=offset;
        if(dir == std::ios_base::cur)
            pos += ((which&std::ios_base::out) ? pptr():gptr()) - buf;
        else if(dir == std::ios_base::end)
            pos += ((which&std::ios_base::out) ? epptr() : egptr()) - buf;
        // else ios_base::beg
        return seekpos(pos, which);
    }
    virtual pos_type seekpos(pos_type pos,
                                            std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
/*        if(pos < 0 || static_cast<size_type>(pos) > size) // allow seeking to pos==size (end)
          return seekpos(pos, which); // invalid*/
        if(which & std::ios_base::out)
            set_ppos(pos);
        if (which & std::ios_base::in)
            set_gpos(pos);
        return pos;
    }
    
    iterator begin() const 
    {
        return buf;
    }
    iterator end() const
    {
        return pptr();
    }
    bool full() const
    {
        return end()==epptr();
    }    
    size_type size() const 
    {
        return end()-begin();
    }
    size_type tellg() const
    {
        return gptr()-begin();
    }
    value_type &back() const 
    {
        return *(end()-1);
    }    
    std::basic_string<cT, traits> str() const {
        return std::basic_string<cT, traits>(begin(), end());
    }
    cT *c_str()
    {
        if (*end()!=traits::eos()) {
            if (full())
                throw "fixed size array_stream full - couldn't NUL-terminate c_str()";
            *end()=traits::eos();
        }
        return begin();
    }
 private:
    using base::pptr;
    using base::pbump;
    using base::epptr;
    using base::gptr;
    using base::egptr;
    using base::gbump;
    basic_array_streambuf(const basic_array_streambuf&);
    basic_array_streambuf& operator=(const basic_array_streambuf&);
 protected:
    virtual self *setbuf(char_type *p,size_type n) 
    {
        set_array(p,n);
        return this;
    }
    char_type *buf, *bufend;
};

typedef basic_array_streambuf<char> array_streambuf;
typedef basic_array_streambuf<wchar_t> warray_streambuf;

template<class cT, class traits = std::char_traits<cT> >
class basic_array_stream : public std::basic_iostream<cT, traits>
{
    typedef std::basic_iostream<cT, traits> base;
    typedef basic_array_streambuf<cT, traits> Sbuf;
 public:
    typedef std::streamsize size_type;    
    typedef cT value_type;
    typedef value_type *iterator; //NOTE: you can count on iterator being a pointer
    typedef basic_array_stream<cT,traits> self;
    
    template <class O>
    void print(O&o) const {o<<m_sbuf;}
    TO_OSTREAM_PRINT
    
    basic_array_stream() : base(&m_sbuf)
    {}
    explicit
    basic_array_stream(const value_type * p, size_type size)
        :	base(&m_sbuf)
    {
        set_array(p,size);
        rdbuf(&m_sbuf);
    }
    template <class C>
    explicit
    basic_array_stream(const C& c)
        :	base(&m_sbuf)
    {
        set_array(c);
        rdbuf(&m_sbuf);
    }    
    inline void set_array(const value_type * p)
    {
        m_sbuf.set_array(p, traits::length(p));
    }
    inline void set_array(const value_type * p, size_type size)
    {
        m_sbuf.set_array(p, size);
    }
    inline void set_array(const std::basic_string<value_type>& s)
    {
        m_sbuf.set_array(s.c_str(), s.size());
    }
    size_type tellg() const 
    {
        return m_sbuf.tellg();    
    }
    Sbuf &buf() 
    {
        return m_sbuf;
    }
    void reset()
    {
        m_sbuf.reset();
        base::clear();
    }
    // allows what was written to be read.
    void reset_read()
    {
        m_sbuf.reset_read();
    }
    // restart reading at the beginning, and hit eof after N read chars
    void reset_read(size_type N)
    {
        m_sbuf.reset_read(N);
    }

    // access to the underlying character array
     iterator begin() const 
    {
        return m_sbuf.begin();
    }
    iterator end() const
    {
        return m_sbuf.end();
    }
    size_type size() const 
    {
        return m_sbuf.size();
    }    
 private:
    Sbuf m_sbuf;
    /*

    
    */
};


typedef basic_array_stream<char> array_stream;
typedef basic_array_stream<wchar_t> warray_stream;

#ifdef TEST
# include "test.hpp"


# define TESTARRAYSTR1 "eruojdcv53341"
# define TESTARRAYSTR2 "0asd"
# define TESTARRAYSTR " " TESTARRAYSTR1 "\n " TESTARRAYSTR2 "\t"
char tarr1[]=TESTARRAYSTR;
using namespace std;

# include <cstring>
template <class C>
void TEST_check_array_stream(C &i1) 
{
    const char *tarrs=TESTARRAYSTR;
    string tstr=tarrs;
    unsigned slen=tstr.length();
    string s;
    BOOST_CHECK(i1>>s);
    BOOST_CHECK(s==TESTARRAYSTR1);
    BOOST_CHECK(i1>>s);
    BOOST_CHECK(s==TESTARRAYSTR2);

/*    BOOST_CHECK(!(i1>>s));
      i1.clear();*/
    
    streambuf::off_type i;
    for (i=0;i<slen;++i) {
        int t;
        i1.seekg(i,ios_base::beg);
        t=i1.tellg();
        BOOST_CHECK(t==(int)i);
        i1.seekg(0);
        BOOST_CHECK(i1.tellg()==0);
        i1.seekg(i);
        t=i1.tellg();
        BOOST_CHECK(t==(int)i);
        int c=i1.get();
        BOOST_CHECK(c==tstr[i]);
        int e=-i-1;
        BOOST_CHECK(i1.seekg(e,ios_base::end));
        t=i1.tellg();
        int p=slen+e;
        BOOST_CHECK(t==p);
        BOOST_CHECK(i1.get()==tstr[p]);
    }
    i1.seekp(0);
    i1.seekg(0);
    for (i=0;i<slen;++i) {        
        BOOST_CHECK((int)i1.tellg()==(int)i);
        BOOST_CHECK((int)i1.tellp()==(int)i);
        BOOST_CHECK(i1.put(i));
        BOOST_CHECK(i==i1.get());
        BOOST_CHECK(i1.begin()[i]==i);
    }
}

template <class C>
inline void TEST_check_memory_stream1(C &o,char *buf,unsigned n) 
{
    const unsigned M=13;
    for(unsigned i=0;i<n;++i) {
        char c=i%M+'a';
//        DBP2(i,c);
        buf[i]=c;
    }
    o.write(buf,n);
    BOOST_REQUIRE(o);
    //    BOOST_CHECK_EQUAL_COLLECTIONS(buf,buf+n,o.begin(),o.end());
    for(unsigned i=0;i<n;++i)
        buf[i]=0;
    /*
    o.read(buf,n);
    BOOST_REQUIRE(o);
    BOOST_CHECK(o.gcount()==n);
    BOOST_CHECK_EQUAL_COLLECTIONS(buf,buf+n,o.begin(),o.end());
    */
}

template <class C>
inline void TEST_check_memory_stream2(C &o,char *buf,unsigned n) 
{
    const unsigned M=13;
    for(unsigned i=0;i<n;++i) {
        BOOST_CHECK(o.put(i%M));
        BOOST_CHECK_EQUAL(o.begin()[i],i%M);
        BOOST_CHECK_EQUAL(o.size(),i+1);
    }
}

template <class C>
inline void TEST_check_memory_stream(C &o,char *buf,unsigned n) 
{
    TEST_check_memory_stream1(o,buf,n);
    o.reset();
    TEST_check_memory_stream2(o,buf,n);
}



BOOST_AUTO_TEST_CASE( TEST_array_stream )
{
    array_stream i1(tarr1);
    TEST_check_array_stream(i1);

    const unsigned N=20;
    char buf[N],buf2[N];

    array_stream o2(buf,N);    
    TEST_check_memory_stream(o2,buf2,N);
}

#endif
}
#endif
