// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef KEY_TO_BLOB_HPP
#define KEY_TO_BLOB_HPP

#include <string>
#include <cstring>

namespace graehl {

/* key type is assumed to be a plain-old-data type (only natural #s work with DB_RECNO)
   Blob (Dbthang) is as specified in Berkeley DB C++ API, but you can plug your own in
   // provides: void key_to_blob(const Key &key,Dbt &dbt) (dbt.set_data(void *);dbt.set_size(Db_size);

   */
template <class Dbthang, class Key>
inline void key_to_blob(const Key &key, Dbthang &dbt)
{
  dbt.set_data((void *)&key);
  dbt.set_size(sizeof(key));
}

template <class Dbthang, class charT, class traits>
inline void key_to_blob(const std::basic_string<charT, traits> &key, Dbthang &dbt)
{
  dbt.set_data((void *)key.data());
  dbt.set_size(key.length()*sizeof(charT));
}

template <class Dbthang>
inline void key_to_blob(const char *key, Dbthang &dbt)
{
  dbt.set_data((void *)key);
  dbt.set_size(std::strlen(key));
}

template <class Key>
struct blob_from_key
{
  //specializations can hold temporary buffer here
  template <class Dbthang>
  blob_from_key(const Key &key, Dbthang &dbt)
  {
    key_to_blob(key, dbt);
  }
};

}

#endif
