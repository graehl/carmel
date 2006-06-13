#ifndef GRAEHL__SHARED__RESERVED_MEMORY_HPP
#define GRAEHL__SHARED__RESERVED_MEMORY_HPP

#include <cstddef>
#include <new>

#ifndef GRAEHL__DEFAULT_SAFETY_SIZE
# define GRAEHL__DEFAULT_SAFETY_SIZE (1024*1024)
# endif
// graehl: when you handle exceptions, you might be out of memory - this little guy grabs extra memory on startup, and frees it on request - of course, if you recover from an exception, you'd better restore the safety (note: you can check the return on consume to see if you really got anything back)

namespace graehl {

struct reserved_memory {
    std::size_t safety_size;
    void *safety;
    reserved_memory(std::size_t safety_size=GRAEHL__DEFAULT_SAFETY_SIZE) : safety_size(safety_size)
    {
        init_safety();
    }
    ~reserved_memory()
    {
        use();
    }
    bool use() {
        if (safety) {
            ::operator delete(safety); // ok if NULL
            safety=0;
            return true;
        } else
            return false;
    }
    void restore(bool allow_exception=true) {
        if (safety)
            return;        
        if (allow_exception)
            init_safety();
        else
            try {
                safety=::operator new(safety_size);
            } catch(...) {
                safety=0;
            }
    }
 private:
    void init_safety() 
    {
        safety=::operator new(safety_size);
    }
};

}//graehl

#endif
