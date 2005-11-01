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
    explicit SafeDb(Db *db_) {
        init(db_);
    }
    void init(Db *db_) 
    {
        init_default_data();
        db=db_;
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
    void close_and_remove()
    {
        close(0);
        /* from API doc:
           The Db::remove method may not be called after calling the Db::open method on any Db handle. If the Db::open method has already been called on a Db handle, close the existing handle and create a new one before calling Db::remove.
        */            
        remove();
    }
    //pre: must have closed db
    void remove()
    {
        assert(db==NULL);
        remove(db_filename);
    }
    
    static void remove(const std::string db_file) 
    {
        Db db(NULL,0);
        db.remove(db_file.c_str(),NULL,0);
    }
    
    void close(Db_flags flags=DB_NOSYNC)
    {
        if (db) {
            db->close(flags);
            delete db;
            db=NULL;
        }        
    }

    // use this to keep the Db object around past SafeDb's lifetime
    Db *disown_db_handle() 
    {
        Db *ret=db;
        db=NULL;
        return ret;
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
        close(0);
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
    inline void put(const Key &key,const Data &data,const char *description="SafeDb::put",Db_flags flags=put_flags_default) 
    {
        to_astr(data);
        put_astr(key,description,flags);
    }
    template <class Key,class Data>    
    inline void get(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        maybe_get(key,data,description,false);
    }
    // returns false if key wasn't found (or fails if allow_notfound was false)
    template <class Key,class Data>    
    inline bool maybe_get(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct",bool allow_notfound=true) 
    {
        if (!allow_notfound)
            maybe_get_astr(key,description,false);
        else
            if (!maybe_get_astr(key,description,true))
                return false;
        from_astr(data);
        return true;
    }

    /////// (PREFERRED) BOOST SERIALIZE STYLE:
    template <class Key,class Data>
    inline void put_via_to_buf(const Key &key,const Data &data,const char *description="SafeDb::put",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);
        Dbt db_data((void*)buf_default,to_buf(data,(void*)buf_default,(Db_size)capacity_default));
        db_try(
            db->put(NULL,&db_key,&db_data,flags),
            description);
    }
    template <class Key,class Data>    
    inline void get_via_from_buf(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        maybe_get_via_from_buf(key,data,description,false);
    }
    // returns false if key wasn't found (or fails if allow_notfound was false)
    template <class Key,class Data>    
    inline bool maybe_get_via_from_buf(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct",bool allow_notfound=true) 
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);
        int ret=db->get(NULL,&db_key,data_for_getting(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);        
        from_buf(data,data_buf(),data_size());
        return true;
    }
    
    template <class Key>
    inline void put_bytes(const Key &key,const void *buf, Db_size buflen,const char *description="SafeDb::put_bytes",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);        
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
        blob_from_key<Key> bk(key,db_key);
        int ret=db->get(NULL,&db_key,data_for_getting(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return 0;
        db_try(ret,description);
        return data_size();
    }

// DIRECT: for plain old data only (no Archive overhead)
    template <class Key,class Data>    
    inline void get_direct(const Key &key,Data *data,const char *description="SafeDb::get_direct") 
    {
        maybe_get(key,data,description,false);
    }
// returns false if key wasn't found
    template <class Key,class Data>    
    inline bool maybe_get_direct(const Key &key,Data *data,const char *description="SafeDb::maybe_get_direct",bool allow_notfound=true) 
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);
        Dbt db_data;
        db_data.set_data(data);
        db_data.set_ulen(sizeof(Data));
        db_data.set_flags(DB_DBT_USERMEM);
        int ret=db->get(NULL,&db_key,&db_data,0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        assert(data_size()==sizeof(Data));
        return true;
    }
//Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_direct(const Key &key,const Data &data,const char *description="SafeDb::put_direct",Db_flags flags=put_flags_default) 
    {        
        put_bytes(key,&data,sizeof(data),description,flags);
    }

    
 private:
    std::string db_filename;
    char buf_default[capacity_default];
    array_stream astr;
    Db *db;
    Dbt _default_data;
    void init_default_data() 
    {
        astr.set_array(buf_default,capacity_default);
        _default_data.set_data(buf_default);
        _default_data.set_flags(DB_DBT_USERMEM);
    }
    Dbt * data_for_getting()
    {
        _default_data.set_ulen(capacity_default);
        return &_default_data;
    }
    Dbt * data_for_putting(Db_size size)
    {
        assert(size <= capacity_default);
        _default_data.set_ulen(size);
        return &_default_data;
    }
    Db_size data_size() const 
    {
        return _default_data.get_size();
    }
    void *data_buf() const
    {
        return (void *)buf_default;
    }
    template <DBTYPE a,u_int32_t b,size_t c>
    SafeDb(const SafeDb<a,b,c> &c) { // since destructor deletes db, this wouldn't be safe
        init(c.db);
        db_filename=c.db_filename;
    }

    //post: astr.begin(),astr.end() holds bytes
    template <class Data>
    inline void to_astr(const Data &d)
    {
        astr.clear();
        default_oarchive a(astr);
        a << d;
    }
    // pre: astr.set_array(data,N)
    template <class Data>
    inline void from_astr(Data *d)
    {        
        default_iarchive a(astr);
        a >> *d;
    }

    template <class Key>
    inline void put_astr(const Key &key,const char *description="SafeDb::put_astr",Db_flags flags=put_flags_default) 
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);        
        db_try(
            db->put(NULL,&db_key,data_for_putting(astr.size()),flags),
            description);
    }
    template <class Key>
    inline bool maybe_get_astr(const Key &key,const char *description="SafeDb::maybe_get_astr",bool allow_notfound=true)
    {
        Dbt db_key;
        blob_from_key<Key> bk(key,db_key);
        int ret=db->get(NULL,&db_key,data_for_getting(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        astr.init_read_size(data_size());
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
        db.close();
        db.remove();
        const char *dbname="tmpdb2";
        
        db.open_create(dbname);
        int k=4;
        int v=10,v2=0;
        db.put_via_to_buf(k,v);
        db.get_via_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        db.close();
        
        db.open_read(dbname);
        v2=0;
        db.get_via_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        v2=0;
        db.get(k,&v2);
        BOOST_CHECK(v2==v);
        BOOST_CHECK(db.maybe_get_bytes(k,buf,buflen));
        int k2=5;
        BOOST_CHECK(!db.maybe_get_bytes(k2,buf,buflen));
        db.close();
        db.open_maybe_create(dbname);
        v2=5;
        db.put_direct(k2,v2);
        int v3=0;
        db.get_direct(k2,&v3);
        BOOST_CHECK(v2==v3);
    }
}
#endif
#endif
