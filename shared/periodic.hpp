#ifndef GRAEHL_SHARED__PERIODIC_HPP
#define GRAEHL_SHARED__PERIODIC_HPP

#include <ostream>

namespace graehl {

struct periodic {
    unsigned period,left;
    bool enabled;
    periodic(unsigned period_=0) {
        set_period(period_);
    }
    void enable() {
        enabled=true;
    }
    void disable() {
        enabled=false;
    }
    void set_period(unsigned period_=0) {
        enabled=true;
        period=period_;
        trigger_reset();
    }
    void trigger_reset() {
        left=period;
    }
    void trigger_next() {
        left=0;
    }
    bool check() {
        if (period && enabled) {
            if (!--left) {
                left=period;
                return true;
            }
        }
        return false;
    }
    void reset()
    {
        enable();
        trigger_reset();
    }
};

template <class C>
struct periodic_wrapper_i : public C
{
    typedef C Imp;
    typedef typename Imp::result_type result_type;
    periodic period;
    unsigned i;
    void reset()
    {
        i=0;
        period.reset();
    }
    periodic_wrapper_i(unsigned period_=0,const Imp &imp_=Imp()) : Imp(imp_),period(period_),i(0) {}
    void set_period(unsigned period_=0)
    {
        period.set_period(period_);
    }
    result_type operator()() {
        ++i;
        if (period.check())
            return Imp::operator()(i);
        return result_type();
    }
    template <class C2>
    result_type operator()(const C2& val) {
        return operator()();
    }
};

struct num_tick_writer
{
    periodic num;
    typedef void result_type;
    std::ostream *pout;
    std::string tick;
    num_tick_writer(unsigned num_each=50) { init(num_each); }
    num_tick_writer(const tick_writer &t) : pout(t.pout),tick(t.tick) {}
    num_tick_writer(std::ostream *o_,const std::string &tick_=".") { init(o_,tick_); }
    num_tick_writer(std::ostream &o_,const std::string &tick_=".") { init(&o_,tick_); }
    void init(std::ostream *o_,const std::string &tick_=".")
    {
        pout=o_;
        tick=tick_;
        init();
    }
    void init(unsigned num_each=50)
    {
        num.set_period(num_each);
    }
    void operator()(unsigned i)
    {
        if (pout) {
            if (num.check())
                *pout<<i<<'\n';
            else
                *pout << tick;
        }
    }
};

typedef periodic_wrapper_i<num_tick_writer> num_ticker;

template <class O>
void num_progress(O &o,unsigned i,unsigned tick_every=10,unsigned num_every_ticks=70,char const* tick=".",char const* post_num="\n",bool tick_and_num=false)
{
    unsigned num_every=tick_every*num_every_ticks;
    if (i % num_every == 0) {
        o << i << post_num;
        if (tick_and_num)
            o << tick;
    } else if (i % tick_every == 0)
        o << tick;
}

}

#endif
