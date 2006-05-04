#ifndef DEFAULT_POOL_ALLOC_HPP
#define DEFAULT_POOL_ALLOC_HPP

namespace graehl {

template <class T,bool destroy=true>
struct default_pool_alloc {
    std::vector <T *> allocated;
    typedef T allocated_type;
    default_pool_alloc() {}
    // move semantics:
    default_pool_alloc(default_pool_alloc<T> &other) : allocated(other.allocated) {
        other.allocated.clear();
    }
    default_pool_alloc(const default_pool_alloc<T> &other) : allocated() {
        assert(other.allocated.empty());
    }
    ~default_pool_alloc() {
        deallocate_all();
    }
    void deallocate_all() {
        for (typename std::vector<T*>::const_iterator i=allocated.begin(),e=allocated.end();i!=e;++i) {
            T *p=*i;
            if (destroy)
                p->~T();
            ::operator delete((void *)p);
        }
    }
    void deallocate(T *p) const {
        // can only deallocate_all()
    }
    T *allocate() {
        allocated.push_back((T *)::operator new(sizeof(T)));
        return allocated.back();
    }
};

}


#endif
