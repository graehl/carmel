// input stream that tracks the current line number
#ifndef LINENOSTREAM_HPP
#define LINENOSTREAM_HPP
#include <steambuf>
#include <istream>

namespace graehl {

struct linenobuf:
  public std::streambuf
{
    linennobuf(std::streambuf* sbuf): m_sbuf(sbuf), m_lineno(1) {}
    int lineno() const { return m_lineno; }

 private:
    int_type underflow() { return m_sbuf->sgetc(); }
    int_type uflow()
    {
        int_type rc = m_sbuf->sbumpc();
        if (traits_type::eq_int_type(rc, traits_type::to_int_type('\n')))
            ++m_lineno;
        return rc;
    }

    std::streambuf* m_sbuf;
    int             m_lineno;
}; 

/* Thanks to Dietmar Kuehl:
   
  Instead of dealing with each
character individually, you can setup a buffer in the 'underflow()'
method (see 'std::streambuf::setg()') which is filled using 'sgetn()'
on the underlying stream buffer. In this case you would not use 'uflow()'
at all (this method is only used for unbuffered input stream buffers).
A huge buffer is read into the internal buffer which is chopped up at
line breaks such that 'underflow()' is called if a line break is hit.
*/

struct ilinenostream:
  public std::istream
{
    ilinenostream(std::istream& stream):
      std::ios(&m_sbuf),
      std::istream(&m_sbuf)
        m_sbuf(stream.rdbuf())
    {
        init(&m_sbuf);
    }

    int lineno() { return m_sbuf.lineno(); }

 private:
    linenobuf m_sbuf;
};

#ifdef GRAEHL_TEST
 #include <fstream>
  int main() {
      return 0;
  }
#endif
}

#endif
