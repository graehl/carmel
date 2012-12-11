#ifndef STRINGABLE_JG201266_HPP
#define STRINGABLE_JG201266_HPP

#include <graehl/shared/string_to.hpp>
#include <graehl/shared/pointer_traits.hpp>
#include <boost/any.hpp>

namespace graehl {

// via string_to and to_string, which default to boost::lexical_cast, but, unlike that, may be overriden for user types other than supporting default/copy ctor and ostream <<, istream >>.


struct stringable
{
  // to/from an implementation pointer
  virtual std::string to_string() const;
  virtual void string_to(std::string const&) = 0;
  virtual void to_string(boost::any const&) const = 0;
  virtual void string_to(std::string const&,boost::any &) const = 0;
  virtual ~stringable() {}
  template <class O>
  void print(O &o) const {
    o<<to_string();
  }
  template <class Ch,class Tr>
  friend std::basic_ostream<Ch,Tr>& operator<<(std::basic_ostream<Ch,Tr> &o, stringable const& self)
  { self.print(o); return o; }

  template <class T> //TODO: use typeid to throw exception when wrong type is used
  friend void string_to(std::string const& s,stringable &me)
  {
    me.string_to(s);
  }
  template <class T> //TODO: use typeid to throw exception when wrong type is used
  friend std::string to_string(stringable const& me)
  {
    return me.to_string();
  }
};

template <class Value>
struct stringable_typed : stringable
{
  typedef Value value_type;
  void to_string(boost::any const& a) const
  {
    return graehl::to_string(boost::any_cast<value_type>(a));
  }
  void string_to(std::string const& s,boost::any &a) const
  {
    a=string_to<value_type>(s);
  }
};

template <class Ptr>
struct ptr_stringable : stringable_typed<typename pointer_traits<Ptr>::element_type>
{
  Ptr p;
  ptr_stringable(Ptr const& p=Ptr()) : p(p) {}
  ptr_stringable(ptr_stringable const& o) : p(o.p) {}
  std::string to_string() const
  {
    return graehl::to_string(*p);
  }
  void string_to(std::string const& s)
  {
    return graehl::string_to(s,*p);
  }
};

}

#endif // STRINGABLE_JG201266_HPP
