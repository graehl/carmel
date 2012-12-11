#ifndef SAFE_DB_HPP
#define SAFE_DB_HPP

#include <db_cxx.h>
#include <stdexcept>
#include <cstddef>
#include <string>
#include <cstdlib>
#include <cstring>
#include <graehl/shared/to_from_buf.hpp>
#include <graehl/shared/key_to_blob.hpp>
#include <graehl/shared/memory_archive.hpp>
#include <boost/config.hpp>
#include <graehl/shared/makestr.hpp>

#ifdef GRAEHL_TEST
# include <graehl/shared/test.hpp>
# include <graehl/shared/debugprint.hpp>
#endif 
//#define DEBUG_SAFEDB

//TODO: use DbEnv (static?) object to allow setting directory for temporary backing files (default is /tmp?)

/* key type is assumed to be a plain-old-data type (only natural #s work with DB_RECNO)    unless you override (see key_to_blob.hpp)
    // key must support key_to_blob: void key_to_blob(const Key &key,Dbt &dbt) (dbt.set_data(void *);dbt.set_size(Db_size);
    
data type is assumed to support serialize method (see Boost.Serialization)
   
NOTE: not finding a key triggers an error/exception unless you call maybe_get
ALSO NOTE: db isn't closed when exception thrown

NOT SUPPORTED: DB_QUEUE
*/

namespace graehl {

typedef u_int32_t Db_size;
typedef db_recno_t Db_recno;

//TODO: single archive for get() put(), with header stored in key:0 (means users don't get to use that key!)
template <DBTYPE DB_TYPE_DEFAULT=DB_RECNO,Db_size PUT_FLAGS=DB_NOOVERWRITE,Db_size MAXBUF=1024*256>
class safe_db 
{
 public:
    typedef u_int32_t Db_flags;
#define SCON(a,b) BOOST_STATIC_CONSTANT(a,b)
    SCON ( Db_size, capacity_default=MAXBUF);

    // put/get flags
    SCON ( Db_flags, put_flags_default = PUT_FLAGS);
    SCON ( Db_flags, overwrite = 0);
    SCON ( Db_flags, no_overwrite = DB_NOOVERWRITE);

    // del_retcode returns
    SCON ( Db_flags, not_found = DB_NOTFOUND);    
    SCON ( Db_flags, key_empty = DB_KEYEMPTY);

    SCON ( DBTYPE, db_type_default = DB_TYPE_DEFAULT);
    SCON ( DBTYPE, RECNO = DB_RECNO);
    SCON ( DBTYPE, HASH = DB_HASH);
    SCON ( DBTYPE, BTREE = DB_BTREE);

    // open_flags:
    SCON ( Db_flags, READONLY = DB_RDONLY);
    SCON ( Db_flags, READWRITE = 0);
    SCON ( Db_flags, MAYBE_CREATE = DB_CREATE);
    SCON ( Db_flags, TRUNCATE = DB_TRUNCATE);
    SCON ( Db_flags, CREATE = MAYBE_CREATE | TRUNCATE);
#undef SCON
    
    DBTYPE db_type_actual() 
    {
        DBTYPE ret;
        db_try(db->get_type(&ret),"safe_db::db_type_actual");
        return ret;
    }
    


    safe_db() 
    {
        init(NULL);
    }
    safe_db(const std::string &filename,Db_flags open_flags=READWRITE,DBTYPE db_type=db_type_default,Db_flags db_flags=0)
    {
        init(NULL);
        open(filename,open_flags,db_type_default,db_flags);
    }
    explicit safe_db(Db *db_) {
        init(db_);
    }
    void init(Db *db_) 
    {
        init_default_data();
        db=db_;
        state=db ? read_and_write : closed;
    }
    operator Db *() const
    {
        return db;
    }    
    inline void db_try(int ret,const char *description="db_try") {
        if (ret!=0) {            
#ifdef DEBUG_SAFEDB
            DBPC4("failure",description,ret,DbEnv::strerror(ret));
#endif 
            throw DbException(description,ret);
        } else {
#ifdef DEBUG_SAFEDB
            DBPC2("success",description);
#endif 
        }
    }

    // synonym to make replacing DiskVector easier
    void seal() 
    {
        sync();
    }
    
    void sync() 
    {
        db_try(db->sync(0),"safe_db::sync");
    }
    int state;
    enum make_not_anon_808636420 {
        closed=0,read_only=1,write=2,read_and_write=read_only&write
    };
    void ok_to_write()
    {
        return db && state!=read_only;
    }    
    void before_write() 
    {
        assert(state!=read_only);
    }

    // must be a DB_RECNO db.  returns recno of appended value
    template <class Val>
    Db_recno append_direct(const Val &val,const char *description="safe_db::append_direct") 
    {
        return append_bytes(&val,sizeof(val),description);
    }
    // must be a DB_RECNO db.  returns recno of appended value
    Db_recno append_bytes(const void *buf, Db_size buflen,const char *description="safe_db::append_bytes") 
    {
        Dbt db_key;
        Dbt db_data((void*)buf,buflen);
        db_try(
            db->put(NULL,&db_key,&db_data,DB_APPEND),
            description);
        return *(Db_recno *)db_key.get_data();
    }
    
    // default: open existing for read/write
    /* db_flags: http://pybsddb.sourceforge.net/api_c/db_set_flags.html#DB_RECNUM
       (DB_DUP, DB_DUPSORT) */
    void open(const std::string &filename,Db_flags open_flags=READWRITE,DBTYPE db_type=db_type_default,Db_flags db_flags=0) 
    {
        assert(db_type!=DB_QUEUE);
        state=(open_flags & READONLY) ? read_only : read_and_write;
        if (!db)
            db=new Db(NULL,0);
        if (db_flags)
            db->set_flags(db_flags);
        db_filename=filename;
        db_try(db->open(NULL, db_filename.c_str(),NULL,db_type,open_flags,0),"safe_db::open");
    }
    void reopen_read(const std::string &filename=db_filename) 
    {
        open_read(db_filename,DB_UNKNOWN);
    }
    void open_read(const std::string &filename,DBTYPE db_type=db_type_default,Db_flags db_flags=0)  
    {
        open(filename,READONLY,db_type,db_flags);
    }
    void open_readwrite(const std::string &filename,DBTYPE db_type=db_type_default,Db_flags db_flags=0) 
    {
        open(filename,READWRITE,db_type,db_flags);
    }
    void open_maybe_create(const std::string &filename,DBTYPE db_type=db_type_default,Db_flags db_flags=0) 
    {
        open(filename,MAYBE_CREATE,db_type,db_flags);
    }
    void open_create(const std::string &filename,DBTYPE db_type=db_type_default,Db_flags db_flags=0) 
    {
        open(filename,CREATE,db_type,db_flags);
    }
    void close_and_delete_dbfile()
    {
        if (db) {            
            close(0);
            /* from API doc:
               The Db::remove method may not be called after calling the Db::open method on any Db handle. If the Db::open method has already been called on a Db handle, close the existing handle and create a new one before calling Db::remove.
            */            
            delete_dbfile();
        }
        
    }
    //pre: must have closed db
    void delete_dbfile()
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
            init(NULL);
        }        
    }

    // use this to keep the Db object around past safe_db's lifetime
    Db *disown_db_handle() 
    {
        Db *ret=db;
        init(NULL);
        return ret;
    }
    
    //!< deletes all records and returns the number so deleted
    Db_size truncate() 
    {
        before_write();
        Db_size ret;
        db->truncate(NULL,&ret,0);
        return ret;
    }
    
    ~safe_db()
    {
        close(0);
    }

    DB_BTREE_STAT btree_stats(bool try_fast=true) 
    {
        DB_BTREE_STAT *btree_stat,ret;
        DBTYPE db_type=db_type_actual();
        if (!(db_type==DB_RECNO || db_type==DB_BTREE))
            throw DbException("wrong db type to get DB_BTREE_STAT");
        bool fast=(try_fast && db_type==DB_RECNO);
        db_try(db->stat(NULL,&btree_stat,fast?DB_FAST_STAT:0),"safe_db::stats");
        ret=*btree_stat;
        std::free(btree_stat);
        return ret;
    }

    DB_HASH_STAT hash_stats(bool try_fast=false)
    {
        DB_HASH_STAT *hash_stat,ret;
        DBTYPE db_type=db_type_actual();
        if (db_type!=DB_HASH)
            throw DbException("wrong db type to get DB_HASH_STAT");
        db_try(db->stat(NULL,&hash_stat,try_fast?DB_FAST_STAT:0),"safe_db::hash_stats");
        ret=*hash_stat;
        std::free(hash_stat);
        return ret;
    }
    

    Db_size get_stat(ptrdiff_t offset_btree=-1,ptrdiff_t offset_hash=-1,bool try_fast_recno=false,bool try_fast_btree=false,bool try_fast_hash=false)
    {
        char *statp;
        Db_size ret;
        DBTYPE db_type=db_type_actual();
        bool fast = false;
        switch(db_type) {
        case DB_BTREE: fast=try_fast_btree;
            break;
        case DB_HASH: fast=try_fast_hash;
            break;
        case DB_RECNO: fast=try_fast_recno;
            break;
        default:
            throw DbException("unsupported DB type (try btree,hash, or recno)");
        }
        db_try(db->stat(NULL,(void*)&statp,(fast?DB_FAST_STAT:0)),"safe_db::n_keys_fast");
        if (db_type==DB_HASH) {
            if (offset_hash<0)
                throw DbException("wrong offset for hash db stats");
            ret=*(Db_size *)(statp+offset_hash);
        } else {
            if (offset_hash<0)
                throw DbException("wrong offset for btree/recno db stats");
            ret=*(Db_size *)(statp+offset_btree);
        }
        std::free(statp);
        return ret;
    }

#define SAFEDB_OFFSETOF(TYPE,FIELD) ((ptrdiff_t)&((TYPE*)0)->FIELD)

    // if you added contiguously without duplicates, should be #of records - fast for RECNO, slow for BTREE
    Db_size n_keys_fast()
    {
        return get_stat(SAFEDB_OFFSETOF(DB_BTREE_STAT,bt_nkeys),
                        SAFEDB_OFFSETOF(DB_HASH_STAT,hash_nkeys),
                        false, // docs say you can use true to make fast for RECNO
                        false,
                        false);
    }
    
    // total number of (key,value) items - always performs full (slow) count
    Db_size n_data_slow()
    {
        return get_stat(SAFEDB_OFFSETOF(DB_BTREE_STAT,bt_ndata),
                        SAFEDB_OFFSETOF(DB_HASH_STAT,hash_ndata),
                        false,
                        false,
                        false);
    }

    // is it an error if you del a nonexistant key?
    template <class Key>
    void del(const Key &key)
    {
        db_try(del_retcode(key),"safe_db::del");
    }

    // returns true if key existed (and was deleted)
    template <class Key>
    bool maybe_del(const Key &key)
    {
        int ret=del_retcode(key);
        if (ret == key_empty || ret == not_found)
            return false;
        db_try(ret,"safe_db::maybe_del");
        return true;
    }
    
#define  MAKE_db_key(key) Dbt db_key;blob_from_key<Key> bk(key,db_key)

    /* The DB->del method will return not_found if the specified key is not in
     * the database. The DB->del method will return key_empty if the database
     * is a Queue or Recno database and the specified key exists */
    template <class Key>
    int del_retcode(const Key &key)
    {
        before_write();
        MAKE_db_key(key);
        return db->del(NULL,&db_key,0);
    }
    
    /////// (PREFERRED) BOOST SERIALIZE STYLE:
    template <class Key,class Data>
    inline void put(const Key &key,const Data &data,Db_flags flags=put_flags_default,const char *description="safe_db::put") 
    {
        before_write();
        array_save(astr,data);
        put_astr(key,flags,description);
    }
    
    template <class Key,class Data>
    inline void release(const Key &key,Data &data) 
    {
    }
    template <class Key,class Data>    
    inline void get(const Key &key,Data *data,const char *description="safe_db::get") 
    {
        maybe_get(key,data,description,false);
    }
    // returns false if key wasn't found (or fails if allow_notfound was false)
    template <class Key,class Data>    
    inline bool maybe_get(const Key &key,Data *data,const char *description="safe_db::maybe_get",bool allow_notfound=true) 
    {
        if (!allow_notfound)
            maybe_get_astr(key,description,false);
        else
            if (!maybe_get_astr(key,description,true))
                return false;
        array_load(astr,*data);
        return true;
    }
    
    /////// (PREFERRED) BOOST SERIALIZE STYLE:
    template <class Key,class Data>
    inline void put_via_to_buf(const Key &key,const Data &data,Db_flags flags=put_flags_default,const char *description="safe_db::put_via_to_buf") 
    {
        before_write();
        MAKE_db_key(key);
        Dbt db_data((void*)buf_default,
                    to_buf(data,(void*)buf_default,(Db_size)capacity_default));
        db_try(
            db->put(NULL,&db_key,&db_data,flags),
            description);
    }
    template <class Key,class Data>    
    inline void get_via_from_buf(const Key &key,Data *data,const char *description="safe_db::get_via_from_buf") 
    {
        maybe_get_via_from_buf(key,data,description,false);
    }
    // returns false if key wasn't found (or fails if allow_notfound was false)
    template <class Key,class Data>    
    inline bool maybe_get_via_from_buf(const Key &key,Data *data,const char *description="safe_db::maybe_get_via_from_buf",bool allow_notfound=true) 
    {
        MAKE_db_key(key);
        int ret=db->get(NULL,&db_key,data_for_getting(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);        
        from_buf(data,data_buf(),data_size());
        return true;
    }
    
    template <class Key>
    inline void put_bytes(const Key &key,const void *buf, Db_size buflen,Db_flags flags=put_flags_default,const char *description="safe_db::put_bytes") 
    {
        before_write();
        MAKE_db_key(key);
        Dbt db_data((void*)buf,buflen);
        db_try(
            db->put(NULL,&db_key,&db_data,flags),
            description);
    }
    template <class Key>
    inline void get_bytes_exact_size(const Key &key,void *buf, Db_size buflen,const char *description="safe_db::get_bytes_exact_size") 
    {
        require_size_equal(buflen,get_bytes(key,buf,buflen,description));            
    }
    
    // returns size actually read (note: if key didn't exist, that's an exception - a 0 size payload is legit)
    template <class Key>
    inline Db_size get_bytes(const Key &key,void *buf, Db_size buflen,const char *description="safe_db::get_bytes") 
    {
        return maybe_get_bytes(key,buf,buflen,description,false);
    }
    
    // returns size of 0 if key wasn't found, size read if found
    template <class Key>
    inline Db_size maybe_get_bytes(const Key &key,void *buf, Db_size buflen,const char *description="safe_db::maybe_get_bytes",bool allow_notfound=true)
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
    inline void get_direct(const Key &key,Data *data,const char *description="safe_db::get_direct") 
    {
        maybe_get_direct(key,data,description,false);
    }
    
    inline static void require_size_equal(Db_size expect,Db_size got,const char *description="safe_db::require_size_equal")
    {
        if (expect == got)
            return;
        std::string s;
        MAKESTR(s,description << ": expected " << expect << " bytes, but got " << got);
        throw DbException(s.c_str());
    }
    
// returns false if key wasn't found
    template <class Key,class Data>    
    inline bool maybe_get_direct(const Key &key,Data *data,const char *description="safe_db::maybe_get_direct",bool allow_notfound=true) 
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
        require_size_equal(sizeof(Data),db_data.get_size());
        return true;
    }
//Data must support: Db_size to_buf(void *&data,Db_size maxdatalen)
    template <class Key,class Data>
    inline void put_direct(const Key &key,const Data &data,Db_flags flags=put_flags_default,const char *description="safe_db::put_direct") 
    {        
        put_bytes(key,&data,sizeof(data),flags,description);
    }
    const std::string &filename() 
    {
        return db_filename;
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
        _default_data.set_size(size);
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
    safe_db(const safe_db<a,b,c> &c) { // since destructor deletes db, this wouldn't be safe
        init(c.db);
        db_filename=c.db_filename;
    }

    //post: astr.begin(),astr.end() holds bytes
    template <class Data>
    inline void to_astr(const Data &d)
    {
        astr.reset();
        default_oarchive a(astr,ARCHIVE_FLAGS_DEFAULT);
        a & d;
#ifdef DEBUG_SAFEDB
/*        ostringstream os;
        default_oarchive dbg_archive(os,ARCHIVE_FLAGS_DEFAULT);
        dbg_archive & d;
        DBP2(os.str(),astr);
*/
        DBPC2("writing",astr);
#endif        
    }

    template <class Key>
    inline void put_astr(const Key &key,Db_flags flags=put_flags_default,const char *description="safe_db::put_astr") 
    {
        before_write();
        MAKE_db_key(key);
#ifdef DEBUG_SAFEDB
        DBP(astr.buf().size());
#endif
        db_try(
            db->put(NULL,&db_key,data_for_putting(astr.buf().size()),flags),
            description);
    }
    template <class Key>
    inline bool maybe_get_astr(const Key &key,const char *description="safe_db::maybe_get_astr",bool allow_notfound=true)
    {
        MAKE_db_key(key);
        int ret=db->get(NULL,&db_key,data_for_getting(),0);
        if (allow_notfound && ret==DB_NOTFOUND)
            return false;
        db_try(ret,description);
        astr.reset_read(data_size());
#ifdef DEBUG_SAFEDB
        astr.buf().set_write_size(data_size());
        DBPC2("read",astr);
#endif 
        return true;
    }
#undef MAKE_db_key
};

#ifdef GRAEHL_TEST
# define CHECKNDATA  BOOST_CHECK_EQUAL(db.n_keys_fast(),n_data);BOOST_CHECK_EQUAL(db.n_keys_fast(),db.n_data_slow())
template <class SDB>
void test_safedb_type()
{
    {
        SDB db;
    }
    {
        unsigned n_data=0;
        const unsigned buflen=10;
        char buf[buflen];
        SDB db;
        db.open_create("tmpdb");
        db.close();
        db.delete_dbfile();
        const char *dbname="tmpdb2";
        
        db.open_create(dbname);
        int k=4;
        int v=10,v2=0;
        db.put_via_to_buf(k,v);
        ++n_data;        CHECKNDATA;
        
        db.get_via_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        db.close();
        
        db.open_read(dbname);
        v2=0;
        db.get_via_from_buf(k,&v2);
        BOOST_CHECK(v2==v);
        db.close();
        db.open(dbname);
        v2=0;
        BOOST_CHECK(db.maybe_del(k));
        --n_data;CHECKNDATA;
        
        BOOST_CHECK(!db.maybe_del(k));
        int delret=db.del_retcode(k);
        BOOST_CHECK(delret==SDB::key_empty || delret==SDB::not_found);
        CHECKNDATA;

        db.put_direct(k,v,SDB::no_overwrite);
        ++n_data;
        CHECKNDATA;
        db.get_direct(k,&v2);
        BOOST_CHECK(v2==v);
        v2=0;
        db.put_direct(k,v,SDB::overwrite);
        db.get_direct(k,&v2);
        BOOST_CHECK(v2==v);
        v2=0;
        for (unsigned i=0;i<2;++i) {
            std::vector<std::string> ss,scopy;
            ss.push_back("a");
            ss.push_back("string - next will be empty");
            ss.push_back("");
            ss.push_back("b");
            
            db.put(k,ss,SDB::overwrite);
            db.get(k,&scopy);
            BOOST_CHECK(ss==scopy);
        }        
        if (1) {
            db.put(k,v,SDB::overwrite);
            db.get(k,&v2);
            BOOST_CHECK(v2==v);
        }
        CHECKNDATA;
        BOOST_CHECK(db.maybe_get_bytes(k,buf,buflen));
        int k2=5;
        BOOST_CHECK(!db.maybe_get_bytes(k2,buf,buflen));
        db.close();
        db.open_maybe_create(dbname);
        v2=5;
        db.put_direct(k2,v2);
        ++n_data;
        
        CHECKNDATA;
        int v3=0;
        db.get_direct(k2,&v3);
        BOOST_CHECK(v2==v3);
    }
}

BOOST_AUTO_TEST_CASE( TEST_safe_db )
{
    test_safedb_type<safe_db<DB_HASH> >();
    test_safedb_type<safe_db<DB_RECNO> >();
}
#endif
}

#endif
