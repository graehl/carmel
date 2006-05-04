#ifndef KEY_TO_BLOB_HPP
#define KEY_TO_BLOB_HPP

#include <string>
#include <cstring>

namespace graehl {

/* key type is assumed to be a plain-old-data type (only natural #s work with DB_RECNO)
   Blob (Dbthang) is as specified in Berkeley DB C++ API, but you can plug your own in
       // provides: void key_to_blob(const Key &key,Dbt &dbt) (dbt.set_data(void *);dbt.set_size(Db_size);
       
 */
template <class Dbthang,class Key>
inline void key_to_blob(const Key &key,Dbthang &dbt) 
{
    dbt.set_data((void *)&key);
    dbt.set_size(sizeof(key));
}

template <class Dbthang,class charT,class traits>
inline void key_to_blob(const std::basic_string<charT,traits> &key,Dbthang &dbt) 
{
    dbt.set_data((void *)key.data());
    dbt.set_size(key.length()*sizeof(charT));
}

template <class Dbthang>
inline void key_to_blob(const char *key,Dbthang &dbt) 
{
    dbt.set_data((void *)key);
    dbt.set_size(std::strlen(key));
}

template <class Key>
struct blob_from_key 
{
    //specializations can hold temporary buffer here
    template <class Dbthang>
    blob_from_key(const Key &key,Dbthang &dbt) 
    {
        key_to_blob(key,dbt);
    }
};

}

#endif
