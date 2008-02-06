#ifndef GRAEHL__SHARED__PUSH_BACKER_HPP
#define GRAEHL__SHARED__PUSH_BACKER_HPP

namespace graehl {

template <class Cont>
struct push_backer
{
    Cont *cont;
    typedef void result_type;
    typedef typename Cont::value_type argument_type;
    typedef push_backer<Cont> self_type;
    push_backer(self_type const& o) : cont(o.cont) {}
    push_backer(Cont &container) : cont(&container) {}
    template <class V>
    void operator()(const V&v) const
    {
        cont->push_back(v);
    }
    void operator()() const
    {
        cont->push_back(argument_type());
    }
};

template <class Cont> inline
push_backer<Cont> make_push_backer(Cont &container) 
{
    return push_backer<Cont>(container);
}

template <class Output_It>
struct outputter
{
    Output_It o;
    typedef void result_type;
//    typedef typename std::iterator_traits<Output_It>::value_type  argument_type;
    typedef outputter<Output_It> self_type;
    outputter(self_type const& o) : o(o.o) {}
    outputter(Output_It const& o) : o(o) {}
    template <class V>
    void operator()(const V&v) const
    {
        *o++=v;
    }
};

template <class Cont> inline
outputter<typename Cont::iterator> make_outputter_cont(Cont &container) 
{
    return outputter<typename Cont::iterator>(container.begin());
}

}

#endif
