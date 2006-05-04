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

template <class Val,class Db_size>
inline Db_size to_buf(const Val &data,void *buf,Db_size buflen) 
{
    assert(buflen >= sizeof(data));
    std::memcpy(buf,&data,sizeof(data));
    return sizeof(data);
}

//DANGER: override this for complex data structures!
template <class Val,class Db_size>
inline Db_size from_buf(Val *pdata,void *buf,Db_size buflen) 
{
    assert(buflen == sizeof(Val));
    std::memcpy(pdata,buf,sizeof(Val));
    return sizeof(Val);
}

template <class Db_size>
inline Db_size to_buf(const std::string &data,void *buf,Db_size buflen) 
{
    Db_size datalen=data.size();    
    if (buflen < datalen)
        throw std::runtime_error("Buffer too small to write string");
    std::memcpy(buf,data.data(),datalen);
    return datalen;
}

//DANGER: override this for complex data structures!
template <class Db_size>
inline Db_size from_buf(std::string *pdata,void *buf,Db_size buflen) 
{
    pdata->assign((char *)buf,buflen);
    return pdata->size()+1;
}


#define MEMBER_TO_FROM_BUF     friend unsigned to_buf(const Self &data,void *buffer, unsigned max_size) \
    { return data.to_buf(buffer,max_size); } \
    friend unsigned from_buf(Self *data,void *buf,unsigned buflen) \
    { return data->from_buf(buf,buflen); }

#endif
