/*
  provides access to a collection of objects larger than virtual memory allows,
  by explicitly mapping a region to disk files which are (re)mapped on demand.
  objects must provide a read method that constructs the object to contiguous
  memory.  input must support seek() to handle out-of-space retries.  your
  object will be stored in memory (saved/loaded) in a fixed location as a POD.
  so you should refer only to heap structures obtained from StackAlloc::alloc*
  in your read(istream &,this,StackAlloc &).  note: if you have no temp/volatile
  fields in your object, then set rw=false in SwapBatch constructor to enforce
  memory protection.

   WARNING: references returned by iterator are potentially invalidated any time
   a different-valued iterator is created or used

   FIXME: real support for random access
*/
#ifndef GRAEHL_SHARED__SWAPBATCH_HPP
#define GRAEHL_SHARED__SWAPBATCH_HPP

#include <graehl/shared/checkpoint_istream.hpp>
#include <graehl/shared/memmap.hpp>
#include <string>
#include <graehl/shared/dynarray.h>
#include <boost/lexical_cast.hpp>
#include <graehl/shared/backtrace.hpp>
#include <graehl/shared/stackalloc.hpp>
#include <graehl/shared/genio.h>
#include <graehl/shared/debugprint.hpp>

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

//FIXME: lifetime mgmt (destructor/constructor?) for contained objects - right now no destructor, and special read() constructor

//FIXME: sometimes .swap.n files left around even though supposed to be deleted.

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
 * StackAlloc requires you to explicitly align<T>() before alloc<T>(). (or use
 * aligned_alloc<T>())
 *
 * void read(istream &in,B &b,StackAlloc &alloc) ... which sets !in (or throws)
 * if input fails, and uses alloc.alloc<T>(n) as space to store an input, returning the
 * new beg - [ret,end) is the new unused range
 */

namespace graehl {

template <class B>
struct SwapBatch {
    typedef SwapBatch<B> Self;

    typedef B BatchMember;
    typedef std::size_t size_type; // boost::intmax_t
    typedef B& reference;
    struct iterator
    {
        typedef SwapBatch<B> Cont;
        typedef B value_type;
        typedef B &reference;
        typedef B *pointer;
        typedef void difference_type; // not implemented but could be unsigned
        typedef std::forward_iterator_tag iterator_category;
        iterator() : cthis(NULL)
        {
        }

        Cont *cthis;
        unsigned batch;
        size_type *header;
        reference operator *() const
        {
            Assert(batch < cthis->n_batch);
            cthis->load_batch(batch);
            return *Cont::data_for_header(header);
        }
        pointer operator ->()
        {
            return &operator *();
        }
        void set_end()
        {
            cthis=NULL;
        }
        bool is_end() const
        {
            return cthis==NULL;
        }
        void operator ++()
        {
//            DBP3(header,batch,cthis->n_batch-1);
            if (*header) {
                header += *header;
            } else {
                if (batch == cthis->n_batch-1) { // equivalently: header == cthis->memmap.begin();
                    set_end();
                    return;
                } else {
                    ++batch;
                    Assert (batch < cthis->n_batch);
                    cthis->load_batch(batch);
                    header=(size_type *)cthis->memmap.begin();
                }
            }
            if (!*header)
                operator++();
        }
        bool operator ==(const iterator &o) const
        {
//            DBPC3("it==",(void *)cthis,(void *)o.cthis);
            Assert (!cthis || !o.cthis || cthis==o.cthis);
            if (o.is_end()  && is_end())
                return true;
            if (!o.is_end() && !is_end())
                return  o.batch==batch && o.header == o.header; // cthis=o.cthis &&
            return false;
//                return cthis==NULL; //!*header && batch = cthis->n_batch-1; // now checked in operator ++
        }
        bool operator !=(const iterator &o) const
        {
            return ! operator==(o);
        }
        operator bool () const  // safe bool
        {
            return !is_end();
        }

        GENIO_print
        {
            if (is_end())
                o << "end";
            else {
                SDBP4(o,cthis,batch,header,operator*());
            }
            return GENIOGOOD;
        }

    };

    GENIO_print
    {
//        SDBP5(o,basename,n_batch,batchsize,loaded_batch,autodelete);
        o << "(\n";
        ((Self *)this)->enumerate(BindWriter<LineWriter,std::basic_ostream<charT,Traits> >(o));
        o << ")\n";
        return GENIOGOOD;
    }

    iterator begin()
    {
        iterator ret;
        if (total_items) {
            ret.cthis=this;
            ret.batch=0;
            ret.header=(size_type*)memmap.begin();
        } else
            return end();
        return ret;
    }

    iterator end()
    {
        iterator ret;
        ret.set_end();
        return ret;
    }

    // bogus operator[] that does sequential scanning (we have no random access index).  should be ok
    reference operator[](unsigned i)
    {
        if (i==0||i<current_i) {
            current_i=0;
            current_iter=begin();
        }
        if (current_iter.is_end()) goto fail;
        while (i>current_i) {
            ++current_i;
            ++current_iter;
            if (current_iter.is_end()) goto fail;
        }
        return *current_iter;
    fail:
        throw std::range_error("index for swapbatch[] is past end");
    }

    bool rw;
    size_type n_batch;
    std::string basename;
    mapped_file memmap;
    size_type batchsize;
    unsigned loaded_batch; // may have to go if we allow memmap sharing between differently-typed batches.
    StackAlloc space;
    bool autodelete;
    void preserve_swap() {
        autodelete=false;
    }
    void autodelete_swap() {
        autodelete=true;
    }
    std::string batch_name(unsigned n) const {
        return basename+boost::lexical_cast<std::string>(n);
    }
/*    void *end() {
        return memmap.end();
    }
    void *begin() {
        return memmap.data();
    }
    const void *end() const {
        return memmap.end();
    }
    const void *begin() const {
        return memmap.data();
        }*/
    size_type capacity() const {
        return memmap.size();
    }
    void create_next_batch() {
        BACKTRACE;
//        DBP_VERBOSE(0);
        DBP_INC_VERBOSE;
        DBPC2("creating batch",n_batch);
        if (n_batch==0) {
            char *base=(char *)DEFAULT_SWAPBATCH_BASE_ADDRESS;
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
            memmap.open(batch_name(n_batch),readmode,batchsize,0,true,base,true); // creates new file and memmaps
        } else {
            memmap.reopen(batch_name(n_batch),readmode,batchsize,0,true); // creates new file and memmaps
        }
        loaded_batch=n_batch++;
        space.init(memmap.begin(),memmap.end());
        d_tail=space.alloc<size_type>(); // guarantee already aligned
        *d_tail=0; // class invariant: each batch has a chain of d_tail diffs ending in 0.
    }
    void load_batch(unsigned i) {
        BACKTRACE;
        DBP_INC_VERBOSE;
        DBPC2("load batch",i);
        if (loaded_batch == i)
            return;
        if (i >= n_batch)
            throw std::range_error("batch swapfile index too large");
        loaded_batch=(unsigned)-1;
        memmap.reopen(batch_name(i),loadmode,batchsize,0,false);
        loaded_batch=i;
    }
    void remove_batches() {
        BACKTRACE;
        memmap.close(); // otherwise windows won't let us delete files
        for (unsigned i=0;i<=n_batch;++i) { // delete next file too in case exception came during create_next_batch
//            DBP2(i,batch_name(i));
            remove_file(batch_name(i));
        }
        n_batch=0; // could loop from n_batch ... 0 but it might confuse :)
    }
    size_t total_items;
    size_t current_i;
    iterator current_iter;
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
    size_type *d_tail; // write here: offset in size_types to next item.  could be an offset in bytes but both fields are size_type aligned for sure.
    BatchMember *read_one(std::istream &is)
    {
        BACKTRACE;
        DBP_ADD_VERBOSE(3);
        std::streampos pos=is.tellg();
        bool first=true;
        void *save;
    again:
        save=space.save_end(); // for reservation:
        BatchMember *newguy;
        try {
            space.alloc_end<size_type>(); // reservation: ensure we can alloc new d_tail.
            DBP(space.remain());
            newguy=space.aligned_alloc<BatchMember>();
            DBP2((void*)newguy,space.remain());
            read((std::istream&)is,*newguy,space);
        }
        catch (StackAlloc::Overflow &o) {
            if (!first)
                throw std::ios::failure("an entire swap space segment couldn't hold the object being read.");
            //ELSE:
            is.seekg(pos);
            create_next_batch();
            first=false;
            goto again;
        }
        if (!is) {
            if (is.eof()) {
                return NULL;
            }
            throw std::ios::failure("error reading item into swap batch.");
        } else {
            DBP2(newguy,space.remain());
        }

        // ELSE: read was success!
        ++total_items;
        DBP2(save,space.end);
        space.restore_end(save); // release d_tail reservation
        DBP2(space.top,space.end);
        size_type *d_last_tail=d_tail;
        Assert(space.capacity<size_type>());
        d_tail=space.aligned_alloc<size_type>(); // can't fail, because of safety
        *d_tail=0; // indicates end of batch; will reset if read is succesful
        *d_last_tail=d_tail-d_last_tail;
        DBP((void*)space.top);
        return newguy;
    }


    void read_all(std::istream &in) {
        BACKTRACE;
        while(in) {
            read_one(in);
        }
    }

    template <class F>
    void read_all_enumerate(std::istream &in,F f) {
        BACKTRACE;
        while(in) {
            BatchMember *newguy=read_one(in);
            if (newguy)
                deref(f)(*newguy);
        }
    }

// reads until eof or delim (which is consumed if it occurs)
    void read_all(std::istream &in,char delim) {
        BACKTRACE;
        char c;
        while(in) {
            BREAK_ONCH_SPACE(delim);
            read_one(in);
        }
    }

    template <class F>
    void read_all_enumerate(std::istream &in,F f,char delim) {
        BACKTRACE;
        char c;
        while(in) {
            BREAK_ONCH_SPACE(delim);
            BatchMember *newguy=read_one(in);
            if (newguy)
                deref(f)(*newguy);
        }
    }

    static BatchMember *data_for_header(const size_type *header)
    {
        BatchMember *b=(BatchMember*)(header+1);
        b=align_up(b);
        return b;
    }

    template <class F>
    void enumerate(F f)  {
        BACKTRACE;
        DBP_INC_VERBOSE;

        for (unsigned i=0;i<n_batch;++i) {
            DBPC2("enumerate batch ",i);
            load_batch(i);
            //=align((size_type *)begin()); // can guarantee first alignment implicitly
            for(const size_type *d_next=(size_type *)memmap.begin();*d_next;d_next+=*d_next) {
                deref(f)(*data_for_header(d_next));
            }
        }
    }

    std::ios::openmode readmode,loadmode;
    SwapBatch(const std::string &basename_,size_type batch_bytesize,bool rw=true) : rw(rw),basename(basename_),batchsize(batch_bytesize), autodelete(true) {
        readmode=rw ? (std::ios::in|std::ios::out) : std::ios::out;
        loadmode=rw ? (std::ios::in|std::ios::out) : std::ios::in;
        BACKTRACE;
        total_items=0;
        n_batch=0;
        if (batchsize < sizeof(size_type))
            batchsize=sizeof(size_type);

        create_next_batch();
    }


    ~SwapBatch() {
        BACKTRACE;
        if (autodelete)
            remove_batches();
    }
};

/*
template <class B>
void read(std::istream &in,B &b,StackAlloc &a) {
    in >> b;
}
*/

void read(std::istream &in,const char * &b,StackAlloc &a) {
    char c;
    std::istream::sentry s(in,true); //noskipws!
//    bool s=true;

    char *p=a.alloc<char>(); // space for the final 0
    b=p;
    if (s) {
        while (in.get(c) && c!='\n'){
            a.alloc<char>(); // space for this char (actually, the next)
            *p++=c;
        }
    }
    *p=0;
    // # alloc<char> = # p increments, + 1.  good.
}

# ifdef TEST
#  include "stdio.h"
#  include "string.h"

const char *swapbatch_test_expect[] = {
    "string one",//10+8
    "2",//2+8
    "3 . ",//4+8
    " abcdefghijklmopqrstuvwxyz",
    "4",
    "end",
    ""
};

static unsigned swapbatch_test_i=0;

void swapbatch_test_do(const char *c) {
    const char *o=swapbatch_test_expect[swapbatch_test_i++];
//    DBP2(o,c);
    BOOST_CHECK(!strcmp(c,o));
}

#  include <sstream>
#  include "os.hpp"

BOOST_AUTO_TEST_CASE( TEST_SWAPBATCH )
{
    using namespace std;
    const char *s1="string one\n2\n3 . \n abcdefghijklmopqrstuvwxyz\n4\nend\n\n";
    string t1=tmpnam(0);
    typedef SwapBatch<const char *> SB;

    BOOST_CHECK(1);

    SB b(t1,28+2*sizeof(size_t)+sizeof(char*)); // string is exactly 28 bytes counting \n
    tmp_fstream i1(s1);
    BOOST_CHECK_EQUAL(b.n_batches(),1);
    BOOST_CHECK_EQUAL(b.size(),0);
    b.read_all((istream&)i1.file);
    BOOST_CHECK_EQUAL(b.n_batches(),5);
    BOOST_CHECK_EQUAL(b.size(),sizeof(swapbatch_test_expect)/sizeof(const char *));
    b.enumerate(swapbatch_test_do);
    BOOST_CHECK_EQUAL(swapbatch_test_i,b.size());
    swapbatch_test_i=0;
    typedef SB::iterator I;
    for (I i=b.begin(),e=b.end();i!=e;++i) {
        swapbatch_test_do(*i);
    }

}

# endif

}

#endif
