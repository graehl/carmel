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
    checkpoint_istream in;
    size_type batchsize;
    char *top; // used only when adding to batches
    static std::string batch_name(unsigned n) {
        return basename+boost::lexical_cast<std::string>(n);
    }
    char *end() const {
        return top;
    }
    char *begin() const {
        return memmap.data();
    }
    size_type capacity() const {
        return memmap.size();
    }
    void create_next_batch() {
        unsigned batch_no = batches.size();
        char *base=(batch_no ? begin() : NULL);
        memmap.close();
        memmap.open(batch_name(batch_no),std::ios::out,batchsize,0,true,base); // creates new file and memmaps
        if (base && base != begin())
            throw ios::failure("couldn't reopen memmap at same base address");
        top = base;
        batches.push_back();
    }
    void load_batch(unsigned i) {
        memmap.close();
        char *base=begin();
        if (i >= batches.size())
            throw std::range_error("batch swapfile index too large");
        memmap.open(batch_name(i),std::ios::in,batchsize,0,false,base); // creates new file and memmaps
        if (!base || base != begin())
            throw ios::failure("couldn't load memmap at same base address");
    }
    /// uses void *B::read(istream &in,void *beg,void *end) ... which sets in.bad() if input fails, and uses [beg,end) as space to store an input, returning the new beg - [ret,end) is the new unused range ... if ret = NULL, then not enough space was available, and should retry; object remaining on failed read should be safe to destroy
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
    void read(istream &in) {
        checkpoint_istream is(in);
        read(is);
    }
    void read(checkpoint_istream &is) {
        char *endspace=begin()+capacity();
        batches.back().push_back();
        char *newtop;
        for (;;) {
            newtop=batches.back().back().read(is,top,endspace);
            if (!is) {
                batches.back().pop_back();
                return;
            }
            if (newtop) {
                Assert(newtop <= endspace);
                top=newtop;
                break;
            } else {
                if (begin() == end()) // already had a fresh batch but still failed
                    throw ios::failure("an entire swap space couldn't hold the object being read.");
                batches.back().pop_back();
                create_next_batch();
            }
        }
    }
    template <class F>
    void enumerate(F f) {
        for (unsigned i=0,end=batches.size();i!=end;++i) {
            load_batch(i);
            Batch &batch=batches[i];
            for (Batch::iterator j=batch.begin(),e=batch.end();j!=end;++j) {
                 deref(f)(*j);
            }
        }
    }
    void readall(istream &in) {
        checkpoint_istream is(in);
        while(is)
            read(is);
    }
    SwapBatch(istream &instd::string basename_,size_type batchsize_) : basename(basename_),batchsize(batchsize_),batch_no(0) {
        create_next_batch();
    }
};

#endif
