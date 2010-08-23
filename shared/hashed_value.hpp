#ifndef GRAEHL__SHARED__HASHED_VALUE_HPP
#define GRAEHL__SHARED__HASHED_VALUE_HPP

#include <boost/functional/hash.hpp>
#include <functional>
#include <stdint.h>
#include <graehl/shared/is_null.hpp>

#ifndef HASHED_VALUE_ASSIGN
# define HASHED_VALUE_ASSIGN 0
#endif

namespace graehl {

template <class Hasher>
struct hash_32 : public Hasher {
  typedef uint32_t result_type;
  typedef Hasher hash_64;

  hash_32() {}
  explicit hash_32(hash_64 const& o) : hash_64(o) {  }

  template <class V>
  result_type operator()(V const& v) const {
    return (result_type)hash_64::operator()(v);
  }
};

// could make this inherit from hash_32<boost::hash<Val> > - but no benefit to doing so.  wanted: C++0x template typedef
template <class Val>
struct bhash_32 : public boost::hash<Val> {
  typedef uint32_t result_type;
  typedef boost::hash<Val> hash_64;

  bhash_32() {}
  explicit bhash_32(hash_64 const& o) : hash_64(o) {  }

  template <class V>
  result_type operator()(V const& v) const {
    return (result_type)hash_64::operator()(v);
  }
};

/// stores a Val which is hashed once by Hasher, with the result stored in type Hasher::result_type (if you only want 32 bits saved, adapt Hasher).  be careful slicing this object to Val and modifying it without rehashing.  it's intended that this only be used at the last moment (i.e. by having a hashing container hold hashed_value instead of just Val).
template <class Val,class Hasher=boost::hash<Val> >
struct hashed_value : public Val
{
  typedef hashed_value<Val,Hasher> self_type;
  typedef typename Hasher::result_type hash_val_type;

  Val const& val() const
  { return *this; }
  Val & val()
  { return *this; }

#if HASHED_VALUE_ASSIGN
  // you may assign to this.
  void operator=(Val const& v2) {
    val()=v2;
    hash=Hasher()(v2);
  }
  void operator=(self_type const& o) {
    val()=o.val();
    hash=o.hash;
  }
#endif
  hashed_value() {  } //WARNING: it's not intended that you ever use a default-init object (hash wasn't computed)
  hashed_value(self_type const& o) : Val(o),hash(o.hash) {}
  hashed_value(Val const &v/*,Hasher const& h=Hasher()*/)
    : Val(v),hash(Hasher()(v)) {}
  hashed_value(Val const &v,Hasher const& h)
    : Val(v),hash(h(v)) {}

  explicit hashed_value(as_null)  {
    set_null();
  }
  void set_null() {
    set_null(val());
    hash=Hasher()(val())+1; // since hash is wrong, nothing can compare equal (in hash table).  of course slicing and comparing to non-hash_value lose this protection
  }

  hash_val_type hash_val() const
  {
    return hash;
  }

  template <class Eq>
  struct equal_to : public std::binary_function<self_type,self_type,bool>
  {
    Eq eq;
    bool operator()(self_type const& a,self_type const&b) const
    {
      return a.equals(b,eq);
    }
    bool operator()(self_type const& a,Val const&b) const
    {
      return eq(a,b); //a.equals(b,eq);
    }
    equal_to(Eq const& eq) : eq(eq) {}
  };
#if 0
  template <class Eq>
  static
  equal_to<Eq> make_equal_to(Eq const& eq)
  {
    return equal_to<Eq>(eq);
  }
#endif

  template <class Eq>
  bool equals(self_type const &v2,Eq const &eq) const
  {
    return hash==v2.hash && eq(val(),v2.val());
  }


  inline bool operator ==(self_type const &v2) const
  {
    return hash==v2.hash && val()==v2.val();
  }
  inline bool operator !=(self_type const &v2) const
  {
    return !(*this==v2);
  }

  template <class Eq>
  bool equals(Val const &v2,Eq const &eq) const
  {
    return eq(val(),v2);
  }
  // compare without hashing.  note: because a hashed_val is a base class Val, if we can determine that the base versions are used rather than the implicit conversion via constructor to hashed_val, then we can comment out the below code:
#if 1
  inline bool operator ==(Val const &v2) const
  {
    return val()==v2;
  }
  inline bool operator !=(Val const &v2) const
  {
    return !(*this==v2);
  }
  friend inline bool operator==(Val const& v2,self_type const& v) {
    return v==v2;
  }
  friend inline bool operator!=(Val const& v2,self_type const& v) {
    return v!=v2;
  }
#endif

  friend inline std::size_t hash_value(self_type const& s)
  {
    return s.hash;
  }
  struct just_hash
  {
    typedef hash_val_type result_type;
    result_type const& operator()(hashed_value<Val,Hasher> const& v) const
    {
      return v.hash_val();
    }
  };

private:
  hash_val_type hash;
};

/*
  // already handled by friend inline above
template <class V,class H>
typename hashed_val<V,H>::hash_val_type hash_value(hashed_val<V,H> const& v) {
  return v.hash_val();
}
*/
}//graehl
#ifdef SAMPLE
# define HASHED_VALUE_SAMPLE
#endif
#ifdef HASHED_VALUE_SAMPLE
# include <iostream>
# include <string>
# include <graehl/shared/show.hpp>
using namespace std;
using namespace boost;
using namespace graehl;
int main(int argc,char *argv[]) {
//  typedef char const* S;
  typedef std::string S;
  bhash_32<S> h32a;
  bhash_32<S> h32=h32a;
  typedef hashed_value<S> V;
  S last="";
  for (int i=1;i<argc;++i)  {
    S s=argv[i];
    V v(s);
# if HASHED_VALUE_ASSIGN
    V w;
    w=v;
# else
    V w=v;
# endif
    SHOW2(SHOWALWAYS,s,last); //,v==w,v==last
    SHOW(SHOWALWAYS,(v==w));
    SHOW(SHOWALWAYS,(v==last));
    SHOW2(SHOWALWAYS,h32(s),h32(v));
    SHOW4(SHOWALWAYS,hash_value(s),hash_value(v),hash_value(w),hash_value(last));
    last=s;
  }
}
#endif

#endif
