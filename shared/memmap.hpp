//wraps memory mapping (supports POSIX and Windows)
// (C) Copyright Jonathan Graehl 2004.
// (C) Copyright Jonathan Turkanis 2004.
// (C) Copyright Craig Henderson 2002.   'boost/memmap.hpp' from sandbox

// License: http://www.boost.org/LICENSE_1_0.txt.)


#ifndef GRAEHL_SHARED__MEMMAP_HPP
#define GRAEHL_SHARED__MEMMAP_HPP
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/backtrace.hpp>
#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

/*
  #include <stdexcept>
  struct memmap_exception : public std::runtime_error {
  char *addr;
  size_t len;
  bool write;
  memmap_exception(char *addr_,size_t len_,bool write_,const std::string &msg="failed") : addr(addr_),len(len_),write(write_),std::runtime_exception(
  std::string("memmap addr=").append(boost::lexical_cast<string>((size_t)addr_)).append(" len=").append(boost::lexical_cast<string>(len_)).append(write_ ? "(read only)":" (read+write)").append(": ").append(msg))
  {}
  };
*/


// Define MEMMAP_IO_SOURCE so that <boost/io/detail/config.hpp> knows that we are
// building the library (possibly exporting code), rather than using it
// (possibly importing code).
#define MEMMAP_IO_SOURCE

#include <cassert>

#include <graehl/shared/os.hpp>

#ifdef MEMMAP_IO_WINDOWS
#else
# include <fcntl.h>
# include <sys/mman.h>      // mmap, munmap.
# include <sys/stat.h>
# include <sys/types.h>     // struct stat.
# include <unistd.h>        // sysconf.
#endif


//
// This header is an adaptation of Craig Henderson's memmory mapped file
// library. The interface has been revised significantly, but the underlying
// OS-specific code is essentially the same.
//
// The following changes have been made:
//
// 1. OS-specific code put in a .cpp file.
// 2. Name of main class changed to mapped_file.
// 3. mapped_file given an interface similar to std::fstream (open(),
//    is_open(), close()) and std::string (data(), size(), begin(), end()).
// 6. Read-only or read-write states are specified using ios_base::openmode.
// 7. Access to the underlying file handles and to security parameters
//    has been removed.
//


#include <boost/config.hpp>                // make sure size_t is in std.
#include <cstddef>                         // size_t.
#include <ios>                             // ios_base::openmode.
#include <boost/cstdint.hpp>               // intmax_t.
//#include <boost/io/detail/config.hpp>      // BOOST_IO_DECL, etc.
#include <boost/noncopyable.hpp>
#include <boost/config/abi_prefix.hpp>     // Must be the last header.


namespace graehl {

struct mapped_file : boost::noncopyable {
private:
    struct safe_bool_helper { int x; };         // From Bronek Kozicki.
    typedef int safe_bool_helper::* safe_bool;
public:
    typedef std::size_t  size_type;
    typedef char*  iterator;
    typedef const char*  const_iterator;
    BOOST_STATIC_CONSTANT(size_type, max_length = static_cast<size_type>(-1));

    mapped_file() : data_(0) , size_(0), mode_(std::ios::openmode()) {}
    mapped_file( const std::string& path,std::ios::openmode mode=std::ios::in | std::ios::out,
                 size_type length = max_length,
                 boost::intmax_t offset = 0,bool create=true,void *base_address=NULL,bool flexible_base=false)
        : data_(0) , size_(0), mode_(mode)
        {
            open(path, mode, length, offset,create,base_address,flexible_base);
        }
    ~mapped_file() {
        close();
    }

    //--------------Stream interface------------------------------------------//

    bool is_open() const { return data_ != 0; }

    operator safe_bool() const { return is_open() ? &safe_bool_helper::x : 0; }
    bool operator!() const { return !is_open(); }

    //--------------Container interface---------------------------------------//

    size_type size() const { return size_; }
    char* data() { return data_; }
    const char* data() const { return data_; }
    iterator begin() { return data(); }
    iterator end() { return data() + size(); }
    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + size(); }

private:

    char*               data_;
    std::size_t         size_;
    std::ios::openmode  mode_;
#ifdef MEMMAP_IO_WINDOWS
    HANDLE  handle_;
    HANDLE  mapped_handle_;
#else
    int     handle_;
#endif

public:
    // atomic convenience routine: create or open same-size, same-base address mapping after closing old one
    void reopen( const std::string& path, std::ios::openmode mode=std::ios::in | std::ios::out,
               size_type length = max_length, boost::intmax_t offset =0,
                 bool create=true)
    {
        void *old_base=data();
        close();
        open(path,mode,length,offset,create,old_base,false);
    }

    void open( const std::string& path,
               std::ios::openmode mode=std::ios::in | std::ios::out,
               size_type length = max_length, boost::intmax_t offset =0,
               bool create=true,void *base_address=NULL,bool flexible_base=false)
        {
            using std::ios;
            DBP_ADD_VERBOSE(1);
            DBPC6("memmap",path,mode,create,base_address,length);
            if (((std::size_t)base_address) & alignment()) {
//                DBP3(base_address,alignment(),((unsigned)base_address) & alignment());
//                throw ios::failure("requested base address for memmap not page-aligned");
                // can't really do this because when we memmap with NULL (OS chooses), it gives us things that aren't aligned (alignment() is buggy)
            }
            bool readonly = (mode & ios::out) == 0;
            if (readonly)
                create=false;
            using namespace std;

            if (is_open())
                throw ios::failure("file already open");
            mode_ = readonly ? ios::in : (ios::in | ios::out);

            boost::intmax_t filesize=offset+length;

#ifdef MEMMAP_IO_WINDOWS //----------------------------------------------------//


            //--------------Open underlying file--------------------------------------//

            handle_ =
                ::CreateFileA( path.c_str(),
                               readonly ? GENERIC_READ : GENERIC_ALL,
                               FILE_SHARE_DELETE, NULL, (create ? CREATE_ALWAYS : OPEN_EXISTING ), FILE_ATTRIBUTE_TEMPORARY, NULL );
            if (handle_ == INVALID_HANDLE_VALUE)
                throw ios::failure(string("couldn't open ").append(path).append(": ").append(last_error_string()));

            //--------------Set file size (if create)---------------------------------//

            if (create) {
#ifdef _MSC_VER
# pragma( push )
# pragma warning (disable : 4244 )
#endif
                LONG sizehigh=(filesize >> (sizeof(LONG) * 8));
                LONG sizelow=(filesize & 0xffffffff);
#ifdef _MSC_VER
# pragma( pop )
#endif

                if (length == max_length ||
                    (::SetFilePointer(handle_,sizelow,&sizehigh,FILE_BEGIN) == INVALID_SET_FILE_POINTER && ::GetLastError()!=NO_ERROR)  ||
                    !::SetEndOfFile(handle_))
                    throw ios::failure(string("couldn't set size to ").append(boost::lexical_cast<std::string>(filesize)).append(": ").append(last_error_string()));
            }

            //--------------Create mapping--------------------------------------------//

            mapped_handle_ =
                ::CreateFileMappingA( handle_, NULL,
                                      readonly? PAGE_READONLY : PAGE_READWRITE,
                                      0, 0, path.c_str() );
            if (mapped_handle_ == NULL) {
                ::CloseHandle(handle_);
                throw ios::failure(string("couldn't create file mapping ").append(path).append(": ").append(last_error_string()));
            }

            //--------------Access data-----------------------------------------------//
          again:
            void* data =
                ::MapViewOfFileEx( mapped_handle_,
                                   readonly ? FILE_MAP_READ : FILE_MAP_WRITE,
                                   (DWORD) (offset >> (sizeof(DWORD) * 8)),
                                   (DWORD) (offset & 0xffffffff),
                                   length != max_length ? length : 0, base_address );
            if (!data) {
                if (base_address && flexible_base) {
                    DBPC2("didn't get preferred base address, retrying at any",last_error_string());
                    base_address=0;
                    goto again;
                }
                ::CloseHandle(mapped_handle_);
                ::CloseHandle(handle_);
                throw ios::failure(string("couldn't map view of file ").append(path).append(": ").append(last_error_string()));
            }

            //--------------Determine file size---------------------------------------//

            LARGE_INTEGER info;
            if (::GetFileSizeEx(handle_, &info)) {
                boost::intmax_t size =
                    ( (static_cast<boost::intmax_t>(info.HighPart) << 32) |
                      info.LowPart );
                size_ =
                    static_cast<std::size_t>( length != max_length ?
                                              std::min<boost::intmax_t>(length, size) :
                                              size );
            } else {
                ::CloseHandle(mapped_handle_);
                ::CloseHandle(handle_);
                throw ios::failure(string("couldn't determine file size of ").append(path).append(": ").append(last_error_string()));
            }

            data_ = reinterpret_cast<char*>(data);

#else // #ifdef MEMMAP_IO_WINDOWS //-------------------------------------------//

            //--------------Open underlying file--------------------------------------//

            int flags = (readonly? O_RDONLY : O_RDWR);
            if (create)
                flags |= (O_CREAT | O_TRUNC);
//            DBP2(path,flags);
            handle_ = ::open(path.c_str(),flags,S_IRWXU) ; //|

            if (handle_ == -1)
                throw ios::failure(string("couldn't open ").append(path).append(": ").append(last_error_string()));

            //--------------Set file size (if create)---------------------------------//

            if (create)
                if (ftruncate(handle_,filesize) ==-1)
                    throw ios::failure(string("couldn't set size to ").append(boost::lexical_cast<std::string>(filesize)).append(": ").append(last_error_string()));


            //--------------Determine file size---------------------------------------//

            bool success = true;
            struct stat info;
            if (length != max_length)
                size_ = length;
            else {
                success = ::fstat(handle_, &info) != -1;
                size_ = info.st_size;
            }
            if (!success) {
                ::close(handle_);
                throw ios::failure(string("couldn't determine file size of ").append(path).append(": ").append(last_error_string()));
            }

            //--------------Create mapping--------------------------------------------//
            // readonly ? MAP_PRIVATE : MAP_SHARED,
          again:
            void* data = ::mmap( base_address, size_,
                                 readonly ? PROT_READ : (PROT_READ|PROT_WRITE),
#ifdef MAP_FILE
                                 MAP_FILE|
#endif
                                 MAP_SHARED|(base_address ? MAP_FIXED : 0),
                                 handle_, offset );
            if (data == MAP_FAILED) {
                if (base_address && flexible_base) {
                    DBPC2("didn't get preferred base address, retrying at any",last_error_string());
                    base_address=0;
                    goto again;
                } else {
                    ::close(handle_);
                    throw ios::failure(string("couldn't mmap file:").append(last_error_string()));
                }
            }
            data_ = reinterpret_cast<char*>(data);
#endif
            DBP((void*)data_);
        }


    bool close()
        {
            bool status;
#ifdef MEMMAP_IO_WINDOWS //----------------------------------------------------//
            if (!is_open()) return true;
            ::SetLastError(0);
            ::UnmapViewOfFile(data_);
            ::CloseHandle(mapped_handle_);
            ::CloseHandle(handle_);
            status=!::GetLastError();
#else
            if (!is_open()) return true;
            errno = 0;
            char *data=reinterpret_cast<char *>(data_);
            ::msync(data, size_, MS_SYNC); // for NFS
            ::munmap(data, size_);
            ::close(handle_);
            status=!errno;
#endif
            data_ = 0;
            return status;
        }
    //--------------Query admissible offsets----------------------------------//

    // Returns the allocation granularity for virtual memory. Values passed
    // as offsets must be multiples of this value.
    //FIXME: buggy both on windows and unix (too pessimistic, gets in the way of remapping to same base as OS gave you before with NULL)

    unsigned alignment()
        {
#ifdef MEMMAP_IO_WINDOWS //----------------------------------------------------//
            SYSTEM_INFO info;
            ::GetSystemInfo(&info);
            return static_cast<unsigned>(info.dwAllocationGranularity);
#else
            return static_cast<unsigned>(sysconf(_SC_PAGESIZE));
#endif
        }

};

#ifdef GRAEHL_TEST
//#include <stdio.h>

BOOST_AUTO_TEST_CASE( TEST_MEMMAP )
{
    using namespace std;
    mapped_file memmap;
    string t1=tmpnam(0);
    string t2=tmpnam(0);
//    DBP2(t1,t2);
    unsigned batchsize=1;
    char *base;
    memmap.close();
    memmap.open(t1,std::ios::out,batchsize,0,true); // creates new file and memmaps
    base=memmap.begin();
    base[0]='z';
    memmap.reopen(t2,std::ios::out,batchsize,0,true); // creates new file and memmaps
    BOOST_CHECK(memmap.begin()==base);
    base[0]='y';
    memmap.reopen(t1,std::ios::in,batchsize,0,false); // opens old
    BOOST_CHECK(memmap.begin()==base);
    BOOST_CHECK(    base[0]=='z');
    memmap.close();
    memmap.open(t2,std::ios::in,batchsize,0,false,base); // creates new file and memmaps
    BOOST_CHECK(memmap.begin()==base);
    BOOST_CHECK(base[0]=='y');
}

#endif

}
#endif
