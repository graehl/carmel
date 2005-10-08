#ifndef SAFE_DB_H
#define SAFE_DB_H

#include <db_cxx.h>
#include <stdexcept>
#include <cstddef>
#include <string>
#include <cstdlib>
#include <cstring>

//TODO: use DbEnv (static?) object to allow setting directory for temporary backing files (default is /tmp?)

/* key type is assumed to be a plain-old-data type (only natural #s work with DB_RECNO)
   for your data type: define global functions
  from_buf(&data,buf,buflen) // (assigns to *data from buflen bytes at buf)
 unsigned to_buf(data,buf,buflen) // (prints to buf up to buflen bytes, returns actual number of bytes written to buf)

 NOTE: not finding a key triggers an error/exception - policy template arg to treat them differently could be added
 ALSO NOTE: db isn't closed when exception thrown
*/

typedef u_int32_t Db_size;

//DANGER: override this for complex data structures!
template <class Val>
inline Db_size to_buf(const Val &data,void *buf,Db_size buflen) 
{
    assert(buflen >= sizeof(data));
    std::memcpy(buf,&data,sizeof(data));
    return sizeof(data);
}

//DANGER: override this for complex data structures!
template <class Val>
inline void from_buf(Val *pdata,void *buf,Db_size buflen) 
{
    assert(buflen == sizeof(Val));
    std::memcpy(pdata,buf,sizeof(Val));
}

template <>
inline Db_size to_buf(const std::string &data,void *buf,Db_size buflen) 
{
    Db_size datalen=data.size();    
    if (buflen < datalen)
        throw DbException("Buffer too small to write string");
    std::memcpy(buf,data.data(),datalen);
    return datalen;
}

//DANGER: override this for complex data structures!
template <>
inline void from_buf(std::string *pdata,void *buf,Db_size buflen) 
{
    pdata->assign((char *)buf,buflen);
}

template <class Dbthang,class Key>
inline void key_to_blob(const Key &key,Dbthang &dbt) 
{
    dbt.set_data((void *)&key);
    dbt.set_size(sizeof(key));
}

template <class Dbthang>
inline void key_to_blob(const std::string &key,Dbthang &dbt) 
{
    dbt.set_data((void *)key.data());
    dbt.set_size(key.size());
}


template <class Dbthang>
inline void key_to_blob(const char *key,Dbthang &dbt) 
{
    dbt.set_data((void *)key);
    dbt.set_size(std::strlen(key));
}


template <DBTYPE DB_TYPE=DB_RECNO,u_int32_t PUT_FLAGS=DB_NOOVERWRITE,size_t MAXBUF=1024*64>
class SafeDb 
{
 public:
    enum {capacity_default=MAXBUF, put_flags_default=PUT_FLAGS };
    static const DBTYPE db_type_default=DB_TYPE;
    
    DBTYPE db_type_actual() 
    {
        DBTYPE ret;
        db_try(db->get_type(&ret),"SafeDb::db_type_actual");
        return ret;
    }
    
    typedef u_int32_t Db_flags;

    SafeDb() 
    {
        init(NULL);
    }
    SafeDb(Db *db_) {
        init(db_);
    }
    void init(Db *db) 
    {                
        default_store();
    }
    operator Db *() const
    {
        return db;
    }
    
    inline void db_try(int ret,const char *description="db_try") {
        if (ret!=0) {
            throw DbException(description,ret);
        }
        
    }

    // default: open existing for read/write
    void open(const std::string &filename,Db_flags flags=0,DBTYPE db_type=db_type_default) 
    {
        if (!db)
            db=new Db(NULL,0);
        db_filename=filename;
        db_try(db->open(NULL, db_filename.c_str(),NULL,db_type,flags,0),"SafeDb::open");
    }    
    void open_read(const std::string &filename,DBTYPE db_type=db_type_default) 
    {
        open(filename,DB_RDONLY,db_type);
    }
    void open_maybe_create(const std::string &filename,DBTYPE db_type=db_type_default)
    {
        open(filename,DB_CREATE,db_type);
    }
    void open_create(const std::string &filename,DBTYPE db_type=db_type_default)
    {
        open(filename,DB_CREATE | DB_TRUNCATE,db_type);
    }
    void close(Db_flags flags=DB_NOSYNC) 
    {
        db->close(flags);
    }
    void close_and_delete()
    {
        close(0);
        db->remove(db_filename.c_str(),NULL,0);
        delete db;
        db=NULL;
    }
    void close_forever()
    {
        if (db) {
            close(0);
            delete db;
            db=NULL;
        }        
    }
    //!< returns number of records removed (all of them)
    Db_size truncate() 
    {
        Db_size ret;
        db->truncate(NULL,&ret,0);
        return ret;
    }
    
    ~SafeDb()
    {
        close_forever();
    }
    
    DB_BTREE_STAT stats(bool try_fast=true) 
    {
        DB_BTREE_STAT btree_stat;
        DBTYPE db_type=db_type_actual();
        if (!(db_type==DB_RECNO || db_type==DB_BTREE))
            throw DbException("wrong db type to get DB_BTREE_STAT");
        bool fast=(try_fast && db_type==DB_RECNO);
        db_try(db->stat(NULL,&stats,fast?DB_FAST_STAT:0),"SafeDb::stats");
        return stats;
    }

    // if you added contiguously without duplicates, should be #of records - fast for RECNO, slow for BTREE
    Db_size n_keys_fast()
    {
        return stats().bt_nkeys;
    }

    // total number of (key,value) items - always performs full (slow) count
    Db_size n_data_slow()
    {
        return stats(false).bt_ndata;
    }
    
    // key must support key_to_blob: void key_to_blob(const Key &key,Dbt &dbt) (dbt.set_data(void *);dbt.set_size(Db_size);
    template <class Key>
    inline void put(const Key &key,const void *buf, Db_size buflen,const char *description="SafeDb::put",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);        
        Dbt db_data((void*)buf,buflen);
        db_try(
            db->put(NULL,&db_key,&db_data,flags),
            description);
    }
// returns size actually read
    template <class Key>
    inline Db_size get(const Key &key,void *buf, Db_size buflen,const char *description="SafeDb::get") 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        Dbt db_data;
        db_data.set_data(buf);
        db_data.set_ulen(buflen);
        db_data.set_flags(DB_DBT_USERMEM);
        db_try(
            db->get(NULL,&db_key,&db_data,0),
            description);
        return db_data.get_size();
    }
    // returns size of 0 if key wasn't found
    template <class Key>
    inline Db_size maybe_get(const Key &key,void *buf, Db_size buflen,const char *description="SafeDb::maybe_get") 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        Dbt db_data;
        db_data.set_data(buf);
        db_data.set_ulen(buflen);
        db_data.set_flags(DB_DBT_USERMEM);
        int ret=db->get(NULL,&db_key,&db_data,0);
        if (ret==DB_NOTFOUND)
            return 0;
        db_try(ret,description);
        return db_data.get_size();
    }
    // Data must support Data(void *data,unsigned datalen)
    template <class Key,class Data>
    inline Data *get_new(const Key &key,const char *description="SafeDb::get_new",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        db_try(
            db->put(NULL,&db_key,&db_data_default,flags),
            description);
        return new Data(buf_default,db_data_default.get_size());
    }
    // Data must support: void from_buf(void *data,Db_size datalen)
    template <class Key,class Data>
    inline void get_from_buf(const Key &key,Data *data,const char *description="SafeDb::get_from_buf",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        db_try(
            db->put(NULL,&db_key,&db_data_default,flags),
            description);
        from_buf(data,buf_default,db_data_default.get_size());
    }
    template <class Key,class Data>    
    inline void get_direct(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        Dbt db_data;
        db_data.set_data(data);
        db_data.set_ulen(sizeof(Data));
        db_data.set_flags(DB_DBT_USERMEM);
        int ret=db->get(NULL,&db_key,&db_data,0);
        db_try(ret,description);
        assert(db_data.get_size()==sizeof(Data));
    }
    //FIXME: make maybe_ a boolean option (inline optimization would be just as good)
    // returns false if key wasn't found
    template <class Key,class Data>    
    inline bool maybe_get_direct(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct") 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        Dbt db_data;
        db_data.set_data(data);
        db_data.set_ulen(sizeof(Data));
        db_data.set_flags(DB_DBT_USERMEM);
        int ret=db->get(NULL,&db_key,&db_data,0);
        if (ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        assert(db_data.get_size()==sizeof(Data));
        return true;
    }
    //Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_to_buf(const Key &key,const Data &data,const char *description="SafeDb::put_to_buf") 
    {
        Db_size size=to_buf(data,buf_default,capacity_default);
        put(key,buf_default,size,description);
    }
    //Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_direct(const Key &key,const Data &data,const char *description="SafeDb::put_to_buf_direct") 
    {
        put(key,&data,sizeof(data),description);
    }
 private:
    std::string db_filename;
    char buf_default[capacity_default];
    Dbt db_data_default;
    Db *db;
    Dbt *default_store() 
    {
        db_data_default.set_data(buf_default);
        db_data_default.set_ulen(capacity_default);
        db_data_default.set_flags(DB_DBT_USERMEM);
        return &db_data_default;
    }
    SafeDb(const SafeDb &c) { // since destructor deletes db, this wouldn't be safe
        init(c.db);
        db_filename=c.db_filename;
    }

};

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_SafeDb )
{
    {
        SafeDb<> db;
    }
    {
        const unsigned buflen=10;
        char buf[buflen];
        SafeDb<> db;
        db.open_create("tmpdb");
        db.close_and_delete();
        const char *dbname="tmpdb2";
        
        db.open_create(dbname);
        int k=4;
        int v=10,v2=0;
        db.put_to_buf(k,v);
        db.get_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        db.close_forever();
        db.open_read(dbname);
        v2=0;
        db.get_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        BOOST_CHECK(db.maybe_get(k,buf,buflen));
        int k2=5;
        BOOST_CHECK(!db.maybe_get(k2,buf,buflen));
        db.close();
        db.open(dbname);
        v2=5;
        db.put_direct(k2,v2);
        int v3=0;
        db.get_direct(k2,&v3);
        BOOST_CHECK(v2==v3);
    }
}
#endif
#endif
