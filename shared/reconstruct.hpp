#ifndef GRAEHL__SHARED__RECONSTRUCT_HPP
#define GRAEHL__SHARED__RECONSTRUCT_HPP

#include <new>


namespace graehl
{

  template <class pointed_to>
  void delete_now(std::auto_ptr<pointed_to> &p) {
//    std::auto_ptr<pointed_to> take_ownership_and_kill(p);
      delete p.release();
  }

  struct delete_anything
  {
      template <class P>
      void operator()(P *p) 
      {
          delete p;
      }    
  };

  template <class V>
  void reconstruct(V &v) {
      v.~V();
      new (&v)V();
  }

  template <class V,class A1>
  void reconstruct(V &v,A1 const& a1) {
      v.~V();
      new (&v)V(a1);
  }

  template <class V,class A1>
  void reconstruct(V &v,A1 & a1) {
      v.~V();
      new (&v)V(a1);
  }

  template <class V,class A1,class A2>
  void reconstruct(V &v,A1 const& a1,A2 const& a2) {
      v.~V();
      new (&v)V(a1,a2);
  }

  template <class V,class A1,class A2>
  void reconstruct(V &v,A1 & a1,A2 const& a2) {
      v.~V();
      new (&v)V(a1,a2);
  }

  template <class V,class A1,class A2>
  void reconstruct(V &v,A1 const& a1,A2 & a2) {
      v.~V();
      new (&v)V(a1,a2);
  }

  template <class V,class A1,class A2>
  void reconstruct(V &v,A1 & a1,A2 & a2) {
      v.~V();
      new (&v)V(a1,a2);
  }
  
}


#endif
