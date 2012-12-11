#ifndef SPARSE_VECTOR_HPP
#define SPARSE_VECTOR_HPP


#include <graehl/shared/hashtable_fwd.hpp>
#include <string>
#include <sstream>
#include <stdexcept>
#include <graehl/shared/string_match.hpp>

namespace graehl {

template <class Key=std::string,class Data=double>
struct sparse_vector : public hash_map<Key,Data>
{
    typedef hash_map<Key,Data> Map;
    typedef std::pair<Key,Data> Component;
    using typename Map::iterator;
    void add_parsing(const std::string &s,char pair_sep=',',char key_val_sep=':') 
    {
        tokenize_key_val_pairs(s,*this,pair_sep,key_val_sep);
    }
    // callback for above
    void operator()(const Component &to_add) 
    {
        pair<typename Map::iterator,bool> add_result=insert(to_add);
        if (!add_result.second)
            throw std::runtime_error("Same key inserted twice into sparse vector");
    }
/*        
        for (string::iterator i=s.begin(),e=s.end();i!=e && *i!=;++i)
            if (*i == pair_sep)
                *i = ' ';
        istringstream is(s);
        string spair;
        while (is >> spair) {
            string::size_type split_pos=spair.find(key_val_sep);
            if (split_pos == string::npos)
                throw runtime_error("Couldn't find key/value separator when parsing sparse vector: "+spair);
            istringstream keys(spair.substring(
        }
*/
    
    
};

typedef sparse_vector<std::string,float> named_sparse_vector;

}

#ifdef GRAEHL_TEST
# include "test.hpp"

BOOST_AUTO_TEST_CASE( TEST_sparse_vector )
{
    named_sparse_vector v1;
    string s="a:1,bc:2,def:0";
    v1.add_parsing(s);
    BOOST_CHECK(v1["a"]==1);
    BOOST_CHECK(v1["def"]==0);
    BOOST_CHECK(v1.size()==3);
    named_sparse_vector(v2);

    v2.add_parsing("");
    BOOST_CHECK(v2.size()==0);
    v2.add_parsing("a:-1");
    BOOST_CHECK(v2.size()==1);
    
    BOOST_CHECK_THROW(v1.add_parsing("bc:-3"),std::runtime_error);
}


#endif 
#endif
