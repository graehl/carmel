#ifndef TEESTREAM_HPP
#define TEESTREAM_HPP
#include <iostream>
 class teebuf: public std::streambuf {
  public:
    typedef std::char_traits<char> traits_type;
    typedef traits_type::int_type  int_type;

    teebuf(std::streambuf* sb1, std::streambuf* sb2):
      m_sb1(sb1),
      m_sb2(sb2)
    {}
    int_type overflow(int_type c) {
      if (m_sb1->sputc(c) == traits_type::eof()
          || m_sb2->sputc(c) == traits_type::eof())
        return traits_type::eof();
      return c;
    }
  private:
    std::streambuf* m_sb1;
    std::streambuf* m_sb2;
  };
 #ifdef TEST
 #include <fstream>
  int main() {
    std::ofstream  logfile("/tmp/logfile.txt");
    teebuf teebuf(std::cout.rdbuf(), logfile.rdbuf());
    std::ostream   log(&teebuf);
    // write log messages to 'log'
    log << "Hello, dude.\n";
  }
 #endif
#endif
