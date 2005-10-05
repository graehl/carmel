#ifndef FIXEDBUFFER_H
#define FIXEDBUFFER_H

#include <stdexcept>
#include <stddef.h>

#ifdef DEBUG
# include <cstring>
# endif
template <class T,bool PlainData=false>
// no bounds checking or growing ...
struct FixedBuffer {
    T *m_begin;
    T *m_end;
    FixedBuffer(size_t sz) : m_begin((T*)::operator new(sizeof(T)*sz)),m_end(m_begin) {
//        INFOL(99,"FixedBuffer","New buffer of " << sz << " elements sized " << sizeof(T) << " bytes.");
# if ASSERT_LVL > 100
        std::memset(m_begin,0x77,sizeof(T)*sz);
#  endif
    }
    operator T *() {
        return m_begin;
    }
    operator const T *() const {
        return m_begin;
    }
    template <class T2>
    inline void push_back(const T2& t) {
        if (PlainData)
            new(m_end++) T(t);
        else
            *m_end++=t;
    }
    inline void push_back() {
        if (PlainData)
            new(m_end++) T();
        else
            ++m_end;
    }
    inline T *push_back_raw() {
        return m_end++;
    }
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() {return m_begin;}
    const_iterator begin() const {return m_begin;}
    iterator end() {return m_end;}
    const_iterator end() const {return m_end;}
    bool empty() const {
        return !size();
    }
    ptrdiff_t size() const {
        return m_end-m_begin;
    }
    ~FixedBuffer() {
        ::operator delete(m_begin);
    }
    void clear() {
        if (PlainData)
            m_end=begin();
        else {
            while(--m_end >= begin())
                m_end->~T();
            ++m_end; // so we're idempotent
        }
    }
    T &at(size_t index) {
        if (index > size())
            throw std::out_of_range();
        return begin()[index];
    }
    const T &at(size_t index) const {
        if (index > size())
            throw std::out_of_range();
        return begin()[index];
    }
};

template <class T,size_t sz,bool PlainData=false>
// no bounds checking or growing ...
struct ConstSizeBuffer {
    char m_begin[sz*sizeof(T)];
    T *m_end;
    ConstSizeBuffer() : m_end((T*)m_begin) {
# if ASSERT_LVL > 100
        std::memset(m_begin,0x77,sizeof(T)*sz);
# endif
    }
    operator T *() {
        return (T*)m_begin;
    }
    operator const T *() const {
        return (const T*)m_begin;
    }
    template <class T2>
    inline void push_back(const T2& t) {
# if ASSERT_LVL > 50
        assert(size()<sz);
#  endif
        if (PlainData)
            new(m_end++) T(t);
        else
            *m_end++=t;
    }
    inline void push_back() {
# if ASSERT_LVL > 50
        assert(size()<sz);
#  endif
        if (PlainData)
            new(m_end++) T();
        else
            push_back_raw();
    }
    inline T *push_back_raw() {
# if ASSERT_LVL > 50
        assert(size()<sz);
#  endif
        return m_end++;
    }
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() {return (iterator)m_begin;}
    const_iterator begin() const {return (const_iterator)m_begin;}
    iterator end() {return m_end;}
    const_iterator end() const {return m_end;}
    bool empty() const {
        return !size();
    }
    size_t size() const {
        return m_end-begin();
    }
    size_t capacity() const {
        return sz;
    }
    T &at(size_t index) {
        if (index > size())
            throw std::out_of_range();
        return begin()[index];
    }
    const T &at(size_t index) const {
        if (index > size())
            throw std::out_of_range();
        return begin()[index];
    }
    void clear() {
        if (PlainData)
            m_end=begin();
        else {
            while(--m_end >= begin())
                m_end->~T();
            ++m_end; // so we're idempotent
        }
    }
};



#endif
