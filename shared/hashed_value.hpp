#ifndef GRAEHL__SHARED__HASHED_VALUE_HPP
#define GRAEHL__SHARED__HASHED_VALUE_HPP

#include <boost/functional/hash.hpp>
#include <functional>

namespace graehl {

/// stores a Val which is hashed once by Hasher, with the result stored in type Hasher::result_type (if you only want 32 bits saved, adapt Hasher)
template <class Val,class Hasher=boost::hash<Val> >
struct hashed_value : public Val
{
    typedef hashed_value<Val,Hasher> self_type;
    
    hashed_value(Val const &v,Hasher const& h=Hasher())
        : Val(v),hash(h(v)) {}
    
    hashed_value(self_type const& o) : Val(o),hash(o.hash) {}
    
    Val const& val() const 
    { return *this; }
    
    Val & val() 
    { return *this; }

    typedef typename Hasher::result_type hash_val_type;
    hash_val_type hash_val() const 
    {
        return hash;
    }
    
    template <class Eq>
    bool equals(self_type const &v2,Eq const &eq) const
    {
        return hash==v2.hash && eq(val(),v2.val());
    }
    
    template <class Eq>
    struct equal_to : public std::binary_function<self_type,self_type,bool>
    {
        Eq eq;
        
        bool operator()(self_type const& a,self_type const&b) const
        {
            return a.equals(b,eq);
        }
        equal_to(Eq const& eq) : eq(eq) {}
    };

    template <class Eq>
    static
    equal_to<Eq> make_equal_to(Eq const& eq)
    {
        return equal_to<Eq>(eq);
    }
        
    template <class Eq>
    bool equals(Val const &v2,Eq const &eq) const
    {
        return hash==v2.hash && eq(val(),v2.val());
    }

    bool operator ==(Val const &v2) const
    {
        return hash==v2.hash && val()==v2.val();
    }
    bool operator !=(Val const &v2) const
    {
        return !(*this==v2);
    }
    friend inline std::size_t hash_value(self_type const& s) 
    {
        return s.hash;
    }
    struct just_hash
    {
        typedef hash_val_type result_type;
        result_type const& operator()(hashed_value<Val,Hasher> const& v) const
        {
            return v.hash_val();
        }
    };    
        
 private:
    hash_val_type hash;
};

}//graehl


#endif
