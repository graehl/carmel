#include <graehl/carmel/src/derivations.h>

namespace graehl {

derivations::state_id derivations::derive(wfst_io_index const& io,deriv_state const& d) 
{
    const int EPS=WFST::epsilon_index;
    state_id ret=g.size();
    
    state_to_id::insert_return_type already=
        id_of_state.insert(d,ret);
    if (!already.second)
        return already.first->second;

    
    add(id_of_state,d,ret); // NOTE: very important that we've added this before we start taking self-epsilons.
    g.push_back();
        
    wfst_io_index::for_state const&fs=io.st[d.s];
        
    add_arcs(io,EPS,EPS,d.i,d.o,fs,ret);
    
    
    if (d.o < out.size()) {
        add_arcs(io,EPS,out[d.o],d.i,d.o+1,fs,ret);
    }
    
    if (d.i < in.size()) {
        Sym i=in[d.i];
        add_arcs(io,i,EPS,d.i+1,d.o,fs,ret);
        if (d.o < out.size()) {
            add_arcs(io,i,out[d.o],d.i+1,d.o+1,fs,ret);
        }
    }

    return ret;    
}


}
