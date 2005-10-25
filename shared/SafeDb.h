#ifndef SAFE_DB_H
#define SAFE_DB_H

#include <db_cxx.h>
#include <stdexcept>
#include <cstddef>
#include <string>
#include <cstdlib>
#include <cstring>
#include "to_from_buf.hpp"
#include "key_to_blob.hpp"
#include "memory_archive.hpp"

//TODO: use DbEnv (static?) object to allow setting directory for temporary backing files (default is /tmp?)

/* key type is assumed to be a plain-old-data type (only natural #s work with DB_RECNO)    unless you override (see key_to_blob.hpp)
    // key must support key_to_blob: void key_to_blob(const Key &key,Dbt &dbt) (dbt.set_data(void *);dbt.set_size(Db_size);
    
data type is assumed to support serialize method (see Boost.Serialization)
   
NOTE: not finding a key triggers an error/exception unless you call maybe_get
ALSO NOTE: db isn't closed when exception thrown
*/

typedef u_int32_t Db_size;

template <DBTYPE DB_TYPE=DB_RECNO,u_int32_t PUT_FLAGS=DB_NOOVERWRITE,size_t MAXBUF=1024*256>
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
        init_default_data();
    }
    operator Db *() const
    {
        return db;
    }    
    inline void db_try(int ret,const char *description="db_try") {
        if (ret!=0)
            throw DbException(description,ret);
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

    
    /////// (PREFERRED) BOOST SERIALIZE STYLE:
    template <class Key,class Data>
    inline void put(const Key &key,const Data &data,const char *description="SafeDb::put") 
    {
        from_data_to_astream(data);
        put_astream(key,description);
    }
    template <class Key,class Data>    
    inline void get(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        maybe_get(key,data,descriptions,false);
    }
    // returns false if key wasn't found (or fails if allow_notfound was false)
    template <class Key,class Data>    
    inline bool maybe_get(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct",bool allow_notfound=true) 
    {
        if (!allow_notfound)
            maybe_get_astream(key,description,false);
        else
            if (!maybe_get_astream(key,description,true))
                return false;
        from_astream_to_data(data);
        return true;
    }
    
    template <class Key>
    inline void put_bytes(const Key &key,const void *buf, Db_size buflen,const char *description="SafeDb::put_bytes",Db_flags flags=put_flags_default) 
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
    inline Db_size get_bytes(const Key &key,void *buf, Db_size buflen,const char *description="SafeDb::get_bytes") 
    {
        return maybe_get_bytes(key,buf,buflen,description,false);
    }
    // returns size of 0 if key wasn't found, size read if found
    template <class Key>
    inline Db_size maybe_get_bytes(const Key &key,void *buf, Db_size buflen,const char *description="SafeDb::maybe_get",bool allow_notfound=true)
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        int ret=db->get(NULL,&db_key,default_get_data(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return 0;
        db_try(ret,description);
        return default_get_size();
    }

// TO_FROM_BUF:
// Data must support: void from_buf(void *data,Db_size datalen)
    template <class Key,class Data>
    inline void get_using_from_buf(const Key &key,Data *data,const char *description="SafeDb::get_using_from_buf",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        db_try(
            db->put(NULL,&db_key,default_get_data(),flags),
            description);
        from_buf(data,buf_default,db_get_data.get_size());
    }
//Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_using_to_buf(const Key &key,const Data &data,const char *description="SafeDb::put_using_to_buf") 
    {
        Db_size size=to_buf(data,buf_default,capacity_default);
        put(key,buf_default,size,description);
    }



// DIRECT: for plain old data only (no Archive overhead)
    template <class Key,class Data>    
    inline void get_direct(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        maybe_get(key,data,descriptions,false);
    }
// returns false if key wasn't found
    template <class Key,class Data>    
    inline bool maybe_get_direct(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct",bool allow_notfound=true) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        Dbt db_data;
        db_data.set_data(data);
        db_data.set_ulen(sizeof(Data));
        db_data.set_flags(DB_DBT_USERMEM);
        int ret=db->get(NULL,&db_key,&db_data,0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        assert(default_get_size()==sizeof(Data));
        return true;
    }
//Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_direct(const Key &key,const Data &data,const char *description="SafeDb::put_direct") 
    {        
        put_bytes(key,&data,sizeof(data),description);
    }

    
 private:
    std::string db_filename;
    char buf_default[capacity_default];
    array_stream astream;
    Db *db;
    Dbt _default_data;
    void init_default_data() 
    {
        astream.set_array(buf_default,capacity_default);
        _default_data.set_data(buf_default);
        _default_data.set_flags(DB_DBT_USERMEM);
    }
    Dbt * default_get_data()
    {
        _default_data.set_ulen(capacity_default);
        return &_default_data;
    }
    Dbt * default_put_data(Db_size size)
    {
        assert(size <= capacity_default);
        _default_data.set_ulen(size);
        return &_default_data;
    }
    Db_size default_get_size() const 
    {
        return _default_data.get_size();
    }
    SafeDb(const SafeDb &c) { // since destructor deletes db, this wouldn't be safe
        init(c.db);
        db_filename=c.db_filename;
    }

    //post: astream.begin(),astream.end() holds bytes
    template <class Data>
    inline void from_data_to_astream(const Data &d)
    {
        astream.clear();
        default_oarchive a(astream);
        a << d;
    }
    // pre: astream.set_array(data,N)
    template <class Data>
    inline void from_astream_to_data(Data *d)
    {
        default_iarchive a(astream);
        a >> *d;
    }
    template <class Key>
    inline void put_astream(const Key &key,const char *description="SafeDb::put_astream",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        key_to_blob(key,db_key);        
        db_try(
            db->put(NULL,&db_key,default_put_data(astream.size()),flags),
            description);
    }
    template <class Key>
    inline bool maybe_get_astream(const Key &key,const char *description="SafeDb::maybe_get_astream",bool allow_notfound=true)
    {
        Dbt db_key;
        key_to_blob(key,db_key);
        int ret=db->get(NULL,&db_key,default_get_data(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        astream.set_size(default_get_size());
        return true;
    }


};

#ifdef TEST
# include "test.hpp"

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
        db.put_using_to_buf(k,v);
        db.get_using_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        db.close_forever();
        db.open_read(dbname);
        v2=0;
        db.get_using_from_buf(k,&v2);
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
