// like unix "tee" for ostreams - see test program below
#ifndef GRAEHL__SHARED__TEESTREAM_HPP
#define GRAEHL__SHARED__TEESTREAM_HPP
#include <iostream>
namespace graehl {
 class teebuf: public std::streambuf {
  public:
    typedef std::char_traits<char> traits_type;
    typedef traits_type::int_type  int_type;

    teebuf(std::streambuf* sb1, std::streambuf* sb2):
      m_sb1(sb1),
      m_sb2(sb2)
    {}
    int_type overflow(int_type c) {
        if ((m_sb1 && m_sb1->sputc(c) == traits_type::eof())
            || (m_sb2 &&m_sb2->sputc(c) == traits_type::eof()))
        return traits_type::eof();
      return c;
    }
  private:
    std::streambuf* m_sb1;
    std::streambuf* m_sb2;
  };
}

 #ifdef GRAEHL_TEST_MANUAL
 #include <fstream>
  int main() {
    std::ofstream  logfile("/tmp/logfile.txt");
    graehl::teebuf teebuf(logfile.rdbuf(),std::cerr.rdbuf());
    std::ostream   log(&teebuf);
    // write log messages to 'log'
    log << "Hello, dude.  check /tmp/logfile.txt\n";
  }
#endif

#endif
