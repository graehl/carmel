// allocate objects within a block contiguously.  memory isn't released to the heap until the block is released.  provides deallocate_all() - good for locally shared sets of objects without needing reference counting/GC
#ifndef PACKEDALLOC_HPP
#define PACKEDALLOC_HPP

#include <graehl/shared/list.h>
#include <memory>

#ifndef PACKED_ALLOC_BLOCKSIZE
#ifdef GRAEHL_TEST
#define PACKED_ALLOC_BLOCKSIZE 8
#else
#define PACKED_ALLOC_BLOCKSIZE 4096
#endif
#endif

namespace graehl {

template <class C,class Alloc=std::allocator<C>,size_t block_size=PACKED_ALLOC_BLOCKSIZE > class PackedAlloc {
    Alloc alloc;
    typedef std::pair<C *,size_t> Block;
    typedef List<Block> ListB;
    ListB blocks;
    C *free_start;
    C *free_end;
public:
    C * allocate(size_t n) {
        if ( free_end >= free_start+n) {
            C *ret=free_start;
            free_start+=n;
            return ret;
        } else {
            C *ret;
            if (n > block_size)  // one-off unique block just big enough
                ret=new_block(n);
            else { // standard sized block with room to spare
                ret=new_block();
                free_start = ret + n;
                free_end = ret + block_size;
            }

            return ret;
        }
    }
    C *new_block(size_t n=block_size) {
        C *ret=alloc.allocate(n);
        blocks.push_front(Block(ret,n));
        return ret;
    }
    void deallocate(C *p, size_t n) {}
    void deallocate_all() {
        for (typename ListB::const_iterator i=blocks.const_begin(),end=blocks.const_end();i!=end;++i)
           alloc.deallocate(i->first,i->second);
    }
    PackedAlloc() {
        free_start=free_end=0;
    }
    ~PackedAlloc() {
        deallocate_all();
    }
};

template <class C,class Alloc=std::allocator<C>,size_t block_size=PACKED_ALLOC_BLOCKSIZE > struct StaticPackedAlloc {
  static PackedAlloc<C,Alloc,block_size> alloc;
  C * allocate(size_t n) {
        return alloc.allocate(n);
  }
  void deallocate(C *p, size_t n) {alloc.deallocate(p,n);}
};

#ifdef GRAEHL__SINGLE_MAIN
template <class C,class Alloc,size_t block_size> PackedAlloc<C,Alloc,block_size> StaticPackedAlloc<C,Alloc,block_size>::alloc;
#endif

}

#endif
