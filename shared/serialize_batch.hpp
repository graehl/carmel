//provides access to a collection of objects larger than virtual memory allows.  input must support seek back to beginning
#ifndef GRAEHL_SHARED__SERIALIZE_BATCH_HPP
#define GRAEHL_SHARED__SERIALIZE_BATCH_HPP

#include <graehl/shared/large_streambuf.hpp>
#include <graehl/shared/simple_serialize.hpp>
#include <graehl/shared/dynarray.h>
//#include <graehl/shared/stream_util.hpp>
#include <boost/config.hpp>

namespace graehl {

/*

intialized as one of:

* with cache filename template (XXXXXX is replaced w/ random string to make a unique filename), then only one value is stored inline in the batch; when the next is built, it's only after serializing the previous to a file for later recall.  the whole collection can be enumerated by rewinding to the beginning and reading the serialized records

* without caching; then the items are built and held in memory, and later enumerated, by the same interface

cursor semantics, so not intended to be multi-thread safe, though we could achieve that in the future with separate read-file-iterators

*/

struct serialize_batch_error : public std::runtime_error
{
    serialize_batch_error() : std::runtime_error("serialize_batch error - didn't read expected RECORD_FOLLOWS or END_RECORDS header") {}
};

struct serialize_batch_index_error : public std::runtime_error
{
    serialize_batch_index_error() : std::runtime_error("serialize_batch error - tried to use record at index >= size") {}
};

template <class B>
struct serialize_batch {
 private:
/*    BOOST_STATIC_CONSTANT(unsigned,RECORD_FOLLOWS=0x31415926);
      BOOST_STATIC_CONSTANT(unsigned,END_RECORDS=0x27182818);
*/
    enum {
        RECORD_FOLLOWS=0x31415926,
        END_RECORDS=0x27182818
    };


 public:
    typedef serialize_batch<B> self_type;

    typedef B value_type;
    typedef std::size_t size_type; // boost::intmax_t

    bool use_file;
    std::string filename;
    std::fstream f;
    istream_archive ia;
    ostream_archive oa;
    bool delete_file; // if true, filename is deleted on destruction
    bigger_streambuf buf;

    value_type current_from_f;

    // if !use_file
    typedef dynamic_array<value_type> A; //FIXME: use slist?  because POD-move might be bad for some people (even though serialize works fine)
    typedef typename A::iterator AI;
    A store;
    AI store_cursor;
    unsigned current_i;

    bool rewind_store; // set to true to rewind, so first advance sets cursor=begin

    size_t total_items;

    bool advance()
    {
        if (use_file) {
            unsigned header;
            ia >> header;
            if (header==END_RECORDS)
                return false;
            else if (header==RECORD_FOLLOWS) {
                ++current_i;
                ia >> current_from_f;
                return true;
            } else
                throw serialize_batch_error();
        } else {
            if (rewind_store) {
                rewind_store=false;
                store_cursor=store.begin();
                current_i=0;
            } else {
                ++current_i;
                ++store_cursor;
            }
            return store_cursor!=store.end();
        }
    }

    void must_advance()
    {
        if (!advance())
            throw serialize_batch_index_error();
    }

    void rewind()
    {
        current_i=(unsigned)-1;
        if (use_file)
            f.seekg(0,std::ios::beg);
        else
            rewind_store=true;
    }

    // may only follow a call to advance() (repeated current() after that is ok).  any changes made won't be preserved for next rewind if using disk.  but i return a mutable reference in case caches in the object need updating.
    value_type &current()
    {
        if (use_file)
            return current_from_f;
        else
            return *store_cursor;
    }

/* to iterate:

for(batch.rewind();batch.advance();)
dosomething(batch.current())
*/
    /*
      GENIO_print
      {
//        SDBP5(o,basename,n_batch,batchsize,loaded_batch,autodelete);
o << "(\n";
((self_type *)this)->enumerate(BindWriter<LineWriter,std::basic_ostream<charT,Traits> >(o));
o << ")\n";
return GENIOGOOD;
}
    */
     size_t size() const {
         return total_items;
     }

    std::string stored_in() const
    {
        return use_file ? filename : "memory";
    }

    void print_stats(std::ostream &out) const {
        out << size() << " items, stored in " << stored_in() << "\n";
    }

    template <class F>
    void enumerate(F f)  {
//        DBP_INC_VERBOSE;
        rewind();
        while(advance())
            deref(f)(current());
    }

    /// i: 0 indexed
    value_type &operator[](unsigned i)
    {
        if (i==0 || i<current_i) {
            rewind();
            must_advance();
        }
        while (current_i<i) must_advance();
        return current();
    }

    // large_bufsize = # of bytes for own fstream buffer, recommend 64*1024*1024
    serialize_batch(bool use_file_,const std::string &filename_,bool delete_file_=true,std::size_t large_bufsize=0)
        : ia(f),oa(f),delete_file(delete_file_),buf(use_file_?large_bufsize:0)
    {
        total_items=0;
        if (use_file_)
            init_file(filename_);
        else
            use_file=false;
    }

    void init_file(std::string const& fn) {
        use_file=true;
        filename=maybe_tmpnam(fn);
        buf.attach_to_stream(f);
        f.open(filename.c_str(),ios::in|ios::out|ios::trunc|ios::binary);
        if (!f.is_open())
            throw serialize_batch_error();
    }

    ~serialize_batch() {
        if (use_file && delete_file) {
            safe_unlink(filename,false);
        }
    }

    /* to populate:
       batch.clear(); // optional on construction
       while (...) {
       value_type &v=batch.start_new(); // note that v may be default initialized (if using vector), or contain the previous value (if using file).  v will in any case have been default constructed initially
       v=1; // or similar to assign/modify.
       if (good(v))
       batch.keep_new();
       else
       batch.drop_new(); // if, in computing the initialization of the start_new()ed item, you decided not to keep it in the batch
       } // done adding things
       batch.mark_end();
    */

    void clear()
    {
        if (use_file) {
            f.seekp(0,std::ios::beg);
        } else {
            store.clear();
        }
        total_items=0;
    }

    value_type &start_new()
    {
        if (use_file)
            return current_from_f;
        else {
            store.push_back();
            return store.back();
        }
    }

    // no more using start_new()'s returned object after one of these:
    void keep_new()
    {
        ++total_items;
        if (use_file) {
            unsigned header=RECORD_FOLLOWS;
            oa << header;
            oa << current_from_f;
        }
    }
    void drop_new()
    {
        if (!use_file)
            store.pop_back();
    }

    // no more calling start_new() after this, until clear()
    void mark_end()
    {
        if (use_file) {
            unsigned header=END_RECORDS;
            oa << header;
            f.flush();
        }
    }

};


# ifdef TEST


BOOST_AUTO_TEST_CASE( TEST_SERIALIZE_BATCH )
{
}

# endif

}

#endif
