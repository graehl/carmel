#ifndef GRAEHL_SHARED__SET_DIFFERENCE_HPP
#define GRAEHL_SHARED__SET_DIFFERENCE_HPP

#include <set>

namespace graehl {

template <class K>
struct set_difference : public std::set<K>
{
    void add(K const& k) 
    {
        this->insert(k);
    }
    bool subtract(K const& k)
    {
        return this->erase(k);
    }
};

    
}

#endif
