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
#ifndef TO_FROM_BUF_HPP
#define TO_FROM_BUF_HPP

#include <string>
#include <cstring>
#include <stdexcept>

/*obsoleted by better Boost.Serialization way of doing things

  for your data type: define global functions
  from_buf(&data,buf,buflen) // (assigns to *data from buflen bytes at buf)
  unsigned to_buf(data,buf,buflen) // (prints to buf up to buflen bytes, returns actual number of bytes written to buf)
*/

//DANGER: override this for complex data structures!


//FIXME: not sure about precedence between MEMBER_TO_FROM_BUF and these if wrap in namespace graehl

template <class Val, class Db_size>
inline Db_size to_buf(const Val &data, void *buf, Db_size buflen)
{
  assert(buflen >= sizeof(data));
  std::memcpy(buf, &data, sizeof(data));
  return sizeof(data);
}

//DANGER: override this for complex data structures!
template <class Val, class Db_size>
inline Db_size from_buf(Val *pdata, void *buf, Db_size buflen)
{
  assert(buflen == sizeof(Val));
  std::memcpy(pdata, buf, sizeof(Val));
  return sizeof(Val);
}

template <class Db_size>
inline Db_size to_buf(const std::string &data, void *buf, Db_size buflen)
{
  Db_size datalen = data.size();
  if (buflen < datalen)
    throw std::runtime_error("Buffer too small to write string");
  std::memcpy(buf, data.data(), datalen);
  return datalen;
}

//DANGER: override this for complex data structures!
template <class Db_size>
inline Db_size from_buf(std::string *pdata, void *buf, Db_size buflen)
{
  pdata->assign((char *)buf, buflen);
  return pdata->size()+1;
}


#define MEMBER_TO_FROM_BUF friend unsigned to_buf(const Self &data, void *buffer, unsigned max_size) \
  { return data.to_buf(buffer, max_size); }                             \
  friend unsigned from_buf(Self *data, void *buf, unsigned buflen)      \
  { return data->from_buf(buf, buflen); }

#endif
