#ifndef GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP
#define GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP

#include <boost/intrusive_ptr.hpp> 
#include <boost/noncopyable.hpp>
#include <boost/detail/atomic_count.hpp>
#include <cassert>

/** usage:
    struct mine : public boost::instrusive_refcount<mine> {};
    mine m;
    boost::intrusive_ptr<mine> p=&m;
*/

namespace boost {
// note: the free functions need to be in boost namespace, OR namespace of involved type.  this is the only way to do it.

template <class T>
class intrusive_refcount;

template <class T>
class atomic_intrusive_refcount;

template<class T>
void intrusive_ptr_add_ref(intrusive_refcount<T>* ptr)
{
    ptr->add_ref();
}

template<class T>
void intrusive_ptr_release(intrusive_refcount<T>* ptr)
{
    ptr->release();
} 


//WARNING: only 2^32 (unsigned) refs allowed.  hope that's ok :)
template<class T>
class intrusive_refcount : boost::noncopyable
{    
 protected:
//    typedef intrusive_refcount<T> pointed_type;
    friend void intrusive_ptr_add_ref<T>(intrusive_refcount<T>* ptr);
    friend void intrusive_ptr_release<T>(intrusive_refcount<T>* ptr);    
//    friend class intrusive_ptr<T>;
    
    intrusive_refcount(): refs(0) {}
    ~intrusive_refcount() { assert(refs==0); }

    void add_ref() { ++refs; }
    void release() { if(!--refs) delete static_cast<T*>(this); }
private:
    unsigned refs;
};


template<class T>
void intrusive_ptr_add_ref(atomic_intrusive_refcount<T>* ptr)
{
    ptr->add_ref();
}

template<class T>
void intrusive_ptr_release(atomic_intrusive_refcount<T>* ptr)
{
    ptr->release();
} 

template<class T>
class atomic_intrusive_refcount : boost::noncopyable
{    
 protected:
    friend void intrusive_ptr_add_ref<T>(atomic_intrusive_refcount<T>* ptr);
    friend void intrusive_ptr_release<T>(atomic_intrusive_refcount<T>* ptr);    
    
    atomic_intrusive_refcount(): refs(0) {}
    ~atomic_intrusive_refcount() { assert(refs==0); }

    void add_ref() { ++refs; }
    void release() { if(!--refs) delete static_cast<T*>(this); }
private:
    boost::detail::atomic_count refs;
};

}


#endif
