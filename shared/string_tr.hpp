#ifndef STRING_TR_HPP
#define STRING_TR_HPP


namespace graehl {

// [] to {}

template <class O,class S,class F>
void write_tr(O &o,S const& s,F map) {
  for (typename S::const_iterator i=s.begin(),e=s.end();i!=e;++i)
    o<<map(*i);
}

template <class S,class F>
S tr(S const& s,F map) {
  S r(s);
  for (typename S::iterator i=s.begin(),e=s.end();i!=e;++i)
    *i=map(*i);
  return r;
}

}



#endif
