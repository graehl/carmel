#ifndef SWAPBATCH_HPP
#define SWAPBATCH_HPP


#include "memmap.hpp"
#include "checkpoint_istream.hpp"
#include <string>
#include "dynarray.h"
#include <boost/lexical_cast.hpp>
#include "backtrace.hpp"

#ifdef HINT_SWAPBATCH_BASE
# ifdef BOOST_IO_WINDOWS
#  ifdef CYGWIN
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0x0A000000U)
#  else
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0x78000000U)
#  endif
# else
#  ifdef SOLARIS
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0xE0000000U)
#  else
#   define VIRTUAL_MEMORY_USER_HEAP_TOP ((void *)0xA0000000U)
#  endif
# endif
# define DEFAULT_SWAPBATCH_BASE_ADDRESS VIRTUAL_MEMORY_USER_HEAP_TOP
// should be NULL but getting odd bad_alloc failure
#else
# define DEFAULT_SWAPBATCH_BASE_ADDRESS ((void *)NULL)
#endif

/*
 * given an input file, a class B with a read method, a batch size (in bytes), and a pathname prefix:
 *
 *  memory maps a region of batch size bytes to files created as prefix+N,
 *  from N=[0,n_batches)

 * On destruction, the files are deleted; without additional specification they
 *wouldn't be portable to future runs (relocatable memory addresses,
 *endian-ness)

 * each batch entry is prefixed by a (aligned - so not indicating the actual
 * semantic content length) size_type d_next; incrementing a size_type pointer by that amount
 * gets you to the next batchentry's header (essentially, a linked list with
 * variable sized data). when the delta is 0, that means the current batch is
 * done, and the next, if any, may be mapped in.  following the d_next header is
 * a region of sizeof(B) bytes, typed as B, of course.  (in this way, forward
 * iteration is supported). no indexing is done to permit random access (except
 * that you may seek to a particular batch), although that could be supported
 * (placing the index at the end or kept in the normal heap).  note that the
 * d_next isn't known until the B is read; the length field is allocated but not
 * initialized until the read is complete.

 * B's read method is free to use any amount of unused batch space (filled from
 * lower addresses->top like a stack).  The read method must acquire it using an
 * (untyped) StackAlloc (which supports, essentially, a bound-checked
 * push_back(N) operation).  Perhaps later I can provide a heap (allowing
 * dealloc/reuse) allocator wrapper that takes a StackAlloc (like sbrk() used in
 * malloc()).

 * When the StackAlloc can't fulfill a request, it throws an exception which is
 * to be caught by SwapBatch; if the batch wasn't empty, a new one can be
 * allocated, the input file read pointer seeked back, and the read retried.  A
 * second failure is permanent (the size of the batch can't be increased, unless
 * we are willing to require a B::relocate() which allows for loading a batch at
 * a different base address; you can't assure a fixed mmap unless you're
 * replacing an old one.
 *
 * StackAlloc requires you to explicitly align<T>() before alloc<T>().
 *
 * void B::read(istream &in,StackAlloc &alloc) ... which sets !in (or throws)
 * if input fails, and uses alloc.alloc<T>(n) as space to store an input, returning the
 * new beg - [ret,end) is the new unused range
 */
#include <stdexcept>


struct StackAlloc
{
    // throws when allocation fails (I don't let you check ahead of time)
    struct Overflow //: public std::exception
    {
    };
    void *top;
    void *end;
    template <class T>
    void align()
    {
/*        unsigned & ttop(*(unsigned *)&top); //FIXME: do we need to warn compiler about aliasing here?
        const unsigned align=boost::alignment_of<T>::value;
        const unsigned align_mask=(align-1);
        unsigned diff=align_mask & ttop; //= align-(ttop&align_mask)
        if (diff) {
            ttop |= align_mask; // = ttop + diff - 1
            ++ttop;
            }*/
        top=align((T*)top);
    }
    template <class T>
    T* alloc()
    {
        return alloc<T>(1); // can't really get much out of explicit specialization (++ vs += 1?)
    }
    template <class T>
    T* aligned_alloc()
    {
        align<T>();
        return alloc<T>(1); // can't really get much out of explicit specialization (++ vs += 1?)
    }
    template <class T>
    T* aligned_alloc(unsigned n)
    {
        align<T>();
        return alloc<T>(n);
    }
    bool full() const
    {
        return top >= end;  // important: different size T will be allocated, and
                         // no guarantee that all T divide space equally (even
                         // for alloc(1)
    }
    template <class T>
    T* alloc(unsigned n)
    {
        T*& ttop(*(T**)&top);
        Assert(is_aligned(ttop));
        T* ret=ttop;
        ttop+=n;
        if (full())
            throw Overflow();
        return ret;
    }
    void save_end(unsigned n)
    {
        end -= n;
    }
    void restore_end(unsigned n)
    {
        end += n;
    }
    void init(void *begin_, void *end_)
    {
        top=begin_;
        end=end_;
    }
};

#ifdef TEST
#include "test.hpp"
BOOST_AUTO_UNIT_TEST( TEST_STACKALLOC )
{
#define N 100
    int a[N];
    StackAlloc s;
    char *top=(char *)a;
    s.init(a,a+N);
    BOOST_CHECK(s.top==a && s.end==a+N);
    s.align<unsigned>();
    BOOST_CHECK(s.top==top);
    BOOST_CHECK(s.alloc<char>()==top && s.top==(top+=1));
    s.align<char>();
    BOOST_CHECK(s.top==top);
    s.align<unsigned>();
    BOOST_CHECK(s.top==(top=(char *)a+sizeof(unsigned)));
    BOOST_CHECK((void *)s.alloc<unsigned>(2)==top && s.top==(top+=2*sizeof(unsigned)));
}

#endif

template <class B>
struct SwapBatch {
    typedef B BatchMember;
    typedef std::size_t size_type; // boost::intmax_t
    size_type n_batch;
    std::string basename;
    mapped_file memmap;
    size_type batchsize;
    unsigned loaded_batch;
    StackAlloc space;
    bool autodelete;
    void preserve_swap() {
        autodelete=false;
    }
    void autodelete_swap() {
        autodelete=true;
    }
    std::string batch_name(unsigned n) {
        return basename+boost::lexical_cast<std::string>(n);
    }
    void *end() {
        return top;
    }
    void *begin() {
        return memmap.data();
    }
    const void *end() const {
        return top;
    }
    const void *begin() const {
        return memmap.data();
    }
    size_type capacity() const {
        return memmap.size();
    }
    void create_next_batch() {
        BACKTRACE;
//        DBP_VERBOSE(0);
        unsigned n_batch = batches.size();
        DBPC2("creating batch",n_batch);
        if (n_batch==0) {
            void *base=DEFAULT_SWAPBATCH_BASE_ADDRESS;
            if (base) {
                const unsigned mask=0x0FFFFFU;
//                DBP4((void *)base,(void*)~mask,(void*)(mask+1),batchsize&(~mask));
                unsigned batchsize_rounded_up=(batchsize&(~mask));
//                DBP2((void*)batchsize_rounded_up,(void*)(mask+1));
                batchsize_rounded_up+=(mask+1);
//                DBP((void*)batchsize_rounded_up);
//                DBP3((void *)base,(void *)batchsize_rounded_up,(void *)(base-batchsize_rounded_up));
                base -= batchsize_rounded_up;
            }
            memmap.open(batch_name(n_batch),std::ios::out,batchsize,0,true,base,true); // creates new file and memmaps
        } else {
            memmap.reopen(batch_name(n_batch),std::ios::out,batchsize,0,true); // creates new file and memmaps
        }
        loaded_batch=n_batch++;
        top = begin();
        batches.push_back();
        space.init(begin(),end());
    }
    void load_batch(unsigned i) {
        BACKTRACE;
        if (loaded_batch == i)
            return;
        if (i >= n_batch)
            throw std::range_error("batch swapfile index too large");
        loaded_batch=(unsigned)-1;
        memmap.reopen(batch_name(i),std::ios::in,batchsize,0,false);
        loaded_batch=i;
    }
    void remove_batches() {
        BACKTRACE;
        for (unsigned i=0;i<=n_batch;++i) { // delete next file too in case exception came during create_next_batch
            remove_file(batch_name(i));
        }
        n_batch=0; // could loop from n_batch ... 0 but it might confuse :)
    }
    size_t total_items=0;
    size_t size() const {
        BACKTRACE;
        return total_items;
    }
    size_t n_batches() const {
        return n_batch;
    }
    void print_stats(std::ostream &out) const {
        out << size() << " items in " << n_batches() << " batches of " << batchsize << " bytes, stored in " << basename << "N";
    }
    bool fresh_batch() const
    {
        return space.top==begin();
    }
    void read(ifstream &is) {
        BACKTRACE;
        DBP_VERBOSE(0);
        std::streampos pos;
        const unsigned safety=2*sizeof(size_type); // leave room for final d_next header (2* to fudge for alignment overhead)
        size_type *d_next=space.alloc<size_type>(); // guarantee that already aligned
        for (;;) {
            pos=is.tellg();
          again:
            try {
                space.save_end(safety);
                BatchMember *newguy=space.aligned_alloc<size_type>();
                newtop=newguy->read(is,space);
            }
            catch (StackAlloc::Overflow &o) {
                if (fresh_batch())
                    throw ios::failure("an entire swap space segment couldn't hold the object being read.");
                //ELSE:
                is.seekg(pos);
                create_next_batch();
                goto again;
            }
            if (is.eof()) {
                return;
            }
            if (!is) {
                throw ios::failure("error reading item into swap batch.");
            }
            // ELSE: read was success!
            ++total_items;
            space.align<size_type>();
            space.restore_end(safety);
            size_type *last_d_next=*d_next;
            *d_next=space.aligned_alloc<size_type>(); // can't fail because of sfaety
            *d_next=0; // indicates end of batch; will reset if read is succesful
            *last_d_next=d_next-last_d_next;
            DBP((void*)space.top);
        }
    }

    template <class F>
    void enumerate(F f) {
        BACKTRACE;
        //      DBP_VERBOSE(0);

        for (unsigned i=0;i<=n_batch;++i) {
            DBPC2("enumerate batch ",i);
            load_batch(i);
                //=align((size_type *)begin()); // can guarantee alignment implicitly
            for(size_type *d_next=(size_type *)begin();*d_next;d_next+=*d_next) {
                BatchMember *b=(BatchMember*)(d_next+1);
                b=align(b);
                deref(f)(*b);
            }
    }

  SwapBatch(const std::string &basename_,size_type batch_bytesize) : basename(basename_),batchsize(batch_bytesize), autodelete(true) {
        BACKTRACE;
        total_items=0;
        create_next_batch();
    }
    ~SwapBatch() {
        BACKTRACE;
        if (autodelete)
            remove_batches();
    }
};

/* OLD


template <class B>
struct SwapBatch {
    typedef B BatchMember;

    typedef DynamicArray<B> Batch;
    typedef DynamicArray<Batch> Batches;
    Batches batches;
    typedef std::size_t size_type; // boost::intmax_t
    std::string basename;
    mapped_file memmap;
    size_type batchsize;
    unsigned loaded_batch;
    char *top; // used only when adding to batches
    bool autodelete;
    void preserve_swap() {
        autodelete=false;
    }
    void autodelete_swap() {
        autodelete=true;
    }
    std::string batch_name(unsigned n) {
        return basename+boost::lexical_cast<std::string>(n);
    }
    char *end() {
        return top;
    }
    char *begin() {
        return memmap.data();
    }
    const char *end() const {
        return top;
    }
    const char *begin() const {
        return memmap.data();
    }
    size_type capacity() const {
        return memmap.size();
    }
    void create_next_batch() {
        BACKTRACE;
//        DBP_VERBOSE(0);
        unsigned batch_no = batches.size();
        DBPC2("creating batch",batch_no);
        if (batch_no==0) {
            char *base=DEFAULT_SWAPBATCH_BASE_ADDRESS;
            if (base) {
                const unsigned mask=0x0FFFFFU;
//                DBP4((void *)base,(void*)~mask,(void*)(mask+1),batchsize&(~mask));
                unsigned batchsize_rounded_up=(batchsize&(~mask));
//                DBP2((void*)batchsize_rounded_up,(void*)(mask+1));
                batchsize_rounded_up+=(mask+1);
//                DBP((void*)batchsize_rounded_up);
//                DBP3((void *)base,(void *)batchsize_rounded_up,(void *)(base-batchsize_rounded_up));
                base -= batchsize_rounded_up;
            }
            memmap.open(batch_name(batch_no),std::ios::out,batchsize,0,true,base,true); // creates new file and memmaps
        } else {
            char *old_base=begin();
            memmap.reopen(batch_name(batch_no),std::ios::out,batchsize,0,true); // creates new file and memmaps
            if (old_base != begin())
                throw ios::failure("couldn't reopen memmap at same base address");
        }
        loaded_batch=batch_no;
        top = begin();
        batches.push_back();
    }
    void load_batch(unsigned i) {
        BACKTRACE;
        if (loaded_batch == i)
            return;
        char *base=begin();
        if (i >= batches.size())
            throw std::range_error("batch swapfile index too large");
        memmap.reopen(batch_name(i),std::ios::in,batchsize,0,false); // creates new file and memmaps
        loaded_batch=(unsigned)-1;
        if (!base || base != begin())
            throw ios::failure("couldn't load memmap at same base address");
        loaded_batch=i;
    }
    void remove_batches() {
        BACKTRACE;
        unsigned batch_no = batches.size();
        for (unsigned i=0;i<=batch_no;++i) { // delete next file too in case exception came during create_next_batch
            remove_file(batch_name(i));
        }
    }
    size_t size() const {
        BACKTRACE;
        size_t total_items=0;
        for (typename Batches::const_iterator i=batches.begin(),end=batches.end();i!=end;++i)
            total_items += i->size();
        return total_items;
    }
    size_t n_batches() const {
        return batches.size();
    }
    void print_stats(std::ostream &out) const {
        out << size() << " items in " << n_batches() << " batches of " << batchsize << " bytes, stored in " << basename << "N";
    }
    void read(ifstream &is) {
        BACKTRACE;
//        DBP_VERBOSE(0);
        char *endspace=begin()+capacity();
        char *newtop;
        std::streampos pos;
        for (;;) {
            pos=is.tellg();
          again:
            batches.back().push_back();
            BatchMember &newguy=batches.back().back();
            newtop=newguy.read(is,top,endspace);
            if (is.eof()) {
                batches.back().pop_back();
                return;
            }

//            DBP((void*)newtop);
            if (!is) {
                throw ios::failure("error reading item into swap batch.");
            }
            if (newtop) {
                Assert(newtop <= endspace);
                top=newtop;
            } else {
                if (begin() == end()) // already had a completely empty batch but still not enough room
                    throw ios::failure("an entire swap space segment couldn't hold the object being read.");
                is.seekg(pos);
                batches.back().pop_back();
                batches.back().compact();
                create_next_batch();
                goto again;
            }
        }
        batches.back().compact();
        batches.compact();
    }

    template <class F>
    void enumerate(F f) {
        BACKTRACE;
        //      DBP_VERBOSE(0);
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            DBPC2("enumerate batch ",i);
            load_batch(i);
            Batch &batch=batches[i];
            for (typename Batch::iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    template <class F>
    void enumerate(F f) const {
        BACKTRACE;
//        DBP_VERBOSE(0);
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            DBPC2("enumerate batch ",i);
            load_batch(i);
            const Batch &batch=batches[i];
            for (typename Batch::const_iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    /// don't actually swap memory (can be useful for statistics that don't look at data)
    template <class F>
    void enumerate_noload(F f) {
        BACKTRACE;
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            Batch &batch=batches[i];
            for (typename Batch::iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    template <class F>
    void enumerate_noload(F f) const {
        BACKTRACE;
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            const Batch &batch=batches[i];
            for (typename Batch::const_iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    SwapBatch(const std::string &basename_,size_type batch_bytesize) : basename(basename_),batchsize(batch_bytesize), autodelete(true) {
        BACKTRACE;
        create_next_batch();
    }
    ~SwapBatch() {
        BACKTRACE;
        if (autodelete)
            remove_batches();
    }
};
*/
#endif
