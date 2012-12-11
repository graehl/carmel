#ifndef DUAL_MEMPOOL_HPP
#define DUAL_MEMPOOL_HPP

#include <graehl/shared/align.hpp>
#include <new>
#include <stdexcept>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

// only works for objects whose size is at least sizeof(void *)
class dual_mempool 
{
    size_t sz1,sz2;
    char *arena_beg,*arena_end;
    typedef void **free_list;
    free_list free1,free2; // NULL terminated free lists
    char *top1,*bot2; //bot2: lowest allocated of sz2-size items; top1: one past highest allocated of sz1-size items
    bool invariant() const 
    {
        return !arena_beg ||  (top1<=bot2 && top1>=arena_beg && bot2>=arena_beg && top1 <= arena_end && bot2 <= arena_end);
    }
    
 public:
    struct Full : public std::bad_alloc,public std::runtime_error {
        using std::runtime_error::what;
        dual_mempool *whatpool;
        Full(const dual_mempool *pool) : std::runtime_error("dual memory pool full"),whatpool(pool) {}
    };
    dual_mempool() : arena_beg(0) {}
    explicit dual_mempool(size_t bytes) 
    {
        alloc_arena(bytes);
    }
    ~dual_mempool()
    {
        dealloc_arena();
    }
    void alloc_arena(size_t bytes) 
    {
        arena_beg=::operator new(bytes);
        arena_end=arena_beg+bytes;
    }    
    void dealloc_arena() 
    {
        ::operator delete(arena_beg);
        arena_beg=NULL;
    }

    void check_overlap() const 
    {
        if (top1>bot2)
            throw Full(this);
    }

    template <class T>
    inline T* overflow(const bool use1)
    {
        Assert2((use1?sz1:sz2),==sizeof(T));
        if (use1) {
            char *ret=top1;
            top1+=sizeof(T);
            check_overlap();
            return (Data *)ret;
        } else {
            bot2-=sizeof(T);
            check_overlap();
            return (Data *)bot2;
        }
    }
            
    template <class T>
    inline T* alloc(const bool use1)
    {
        Assert2((use1?sz1:sz2),==sizeof(T));
        free_list *free=use1?&free1:&free2;
        if (*free) {
            free_list ret=*free;            
            *free=(free_list)**free; // next pointer
            return (T*)ret;
        }
        return overflow(use1);
    }

    template <class T>
    inline void free(T *data_from_arena,const bool use1)
    {
        Assert2((use1?sz1:sz2),==sizeof(T));
        Assert(inbounds(data_from_arena,use1));
        free_list *free=use1?&free1:&free2;
        free_list newhead=(free_list)data_from_arena;
        *data_from_arena=*free;
        *free=data_from_arena;
    }
    
    inline bool inbounds(void *data_from_arena,const bool use1)
    {
        void *lower=use1?arena_beg:bot2;
        void *upper=use1?top1:arena_end;
        return (lower<=data_from_arena && upper>data_from_arena);
    }
        

    
    
        

    
/*
    struct first { enum make_not_anon_483272012 { DIRECTION=1 }; };    
    struct second { enum make_not_anon_490088557 { DIRECTION=-1 }; };
    
    template <class Type>
    size_t element_size() const;    
    template <>
    size_t element_size<first>() const 
    {
        return sz1;
    }
    template <>
    size_t element_size<second>() const 
    {
        return sz2;
    }

    template <>
    size_t n_allocated<first>() const 
    {
        return (top1-arena_beg)/sz1;
    }
    template <>
    size_t n_allocated<second>() const 
    {
        return (arena_end-bot1)/sz2;
    }


    template <class Type,class Data>
    Data *overflow() const;    
    template <class Data>
    Data *overflow<first>() const 
    {
        Assert2(sz1,=sizeof(Data));
        char *ret=top1;
        top1+=sz1;
        check_overlap();
        return (Data *)ret;
    }
    template <class Data>
    Data *overflow<second>() const 
    {
        Assert2(sz2,=sizeof(Data));
        bot2-=sz2;
        check_overlap();
        return (Data *)bot2;
    }

    template <class Type,class Data>
    void reset_arena() const ;    
    template <class Data>
    void reset_arena<first>() const 
    {
        sz1=sizeof(Data);
        top1=align((Data *)arena_beg);
    }
    template <class Data>
    void reset_arena<second>() const 
    {
        sz2=sizeof(Data);
        bot2=align_down((Data *)arena_end);
    }

    template <class Type,class Data>
    Data *allocate();
    
    
    template <class Data>
    Data *allocate<first>()
    {
Assert2(sz1,=sizeof(Data));
    }
*/    
    
    

};

    

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_DUAL_MEMPOOL )
{
    BOOST_CHECK_EQUAL(1,0);
}
#endif

}

#endif
