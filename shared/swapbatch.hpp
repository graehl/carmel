#ifndef SWAPBATCH_HPP
#define SWAPBATCH_HPP

#include "memmap.hpp"
#include "checkpoint_istream.hpp"
#include <string>
#include "dynarray.h"
#include <boost/lexical_cast.hpp>

template <class B>
struct SwapBatch {
    typedef B BatchMember;
    typedef DynamicArray<B> Batch;
/*    struct {
        std::string memmap_file;
        Batch batch;
        } BatchWhere;*/
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
//        DBP_VERBOSE(0);
        unsigned batch_no = batches.size();
        DBPC2("creating batch",batch_no);
        char *base=(batch_no ? begin() : NULL);
        memmap.close();
        memmap.open(batch_name(batch_no),std::ios::out,batchsize,0,true,base); // creates new file and memmaps
        if (base && base != begin())
            throw ios::failure("couldn't reopen memmap at same base address");
        loaded_batch=batch_no;
        top = begin();
        batches.push_back();
    }
    void load_batch(unsigned i) {
        if (loaded_batch == i)
            return;
        char *base=begin();
        memmap.close();
        if (i >= batches.size())
            throw std::range_error("batch swapfile index too large");
        memmap.open(batch_name(i),std::ios::in,batchsize,0,false,base); // creates new file and memmaps
        loaded_batch=i;
        if (!base || base != begin())
            throw ios::failure("couldn't load memmap at same base address");
    }
    void remove_batches() {
        unsigned batch_no = batches.size();
        for (unsigned i=0;i<batch_no;++i) {
            remove_file(batch_name(i));
        }
    }
    /// uses void *B::read(istream &in,void *beg,void *end) ... which sets !in if input fails, and uses [beg,end) as space to store an input, returning the new beg - [ret,end) is the new unused range ... if ret = NULL, then not enough space was available, and should retry; object remaining on failed read should be safe to destroy
    /*
      e.g. struct B{
      template <class I>
    void *read(I &in,void *beginspace,void *endspace) {
        begin=beginspace;
        end=endspace;
        in >> *this;
        if (ran_out_of_space())
            return NULL;
        else
            return end;
    }
    }; */
    size_t size() const {
        size_t total_items=0;
        for (typename Batches::const_iterator i=batches.begin(),end=batches.end();i!=end;++i)
            total_items += i->size();
        return total_items;
    }
    /*
    void read(istream &in) {
        checkpoint_istream_control<fstream> transact;
        typename checkpoint_istream_control<fstream>::istream_wrapper fin(*(fstream *)&in);
        read(fin,transact);
    }

    template <class In,class InTransactor>
    void read(In &is,InTransactor &transact) {
        char *endspace=begin()+capacity();
        batches.back().push_back();
        char *newtop;
        transact.commit(deref(is));
        for (;;) {
            newtop=batches.back().back().read(deref(is),top,endspace);
            if (!deref(is)) {
                batches.back().pop_back();
                return;
            }
            if (newtop) {
                Assert(newtop <= endspace);
                top=newtop;
                transact.commit(deref(is));
            } else {
                if (begin() == end()) // already had a completely empty batch but still not enough room
                    throw ios::failure("an entire swap space segment couldn't hold the object being read.");
                transact.revert(deref(is));
                batches.back().pop_back();
                create_next_batch();
            }
        }
    }
    */
    void read(ifstream &is) {
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

            DBP((void*)newtop);
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
                create_next_batch();
                goto again;
            }
        }
    }

    template <class F>
    void enumerate(F f) {
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
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            Batch &batch=batches[i];
            for (typename Batch::iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    template <class F>
    void enumerate_noload(F f) const {
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            const Batch &batch=batches[i];
            for (typename Batch::const_iterator j=batch.begin(),endj=batch.end();j!=endj;++j) {
                 deref(f)(*j);
            }
        }
    }

    SwapBatch(const std::string &basename_,size_type batch_bytesize) : basename(basename_),batchsize(batch_bytesize), autodelete(true) {
        create_next_batch();
    }
    ~SwapBatch() {
        if (autodelete)
            remove_batches();
    }
};

#endif
