//-----------------------------------------------------------------------------
// foreach.hpp header file
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Niebler
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.
//-----------------------------------------------------------------------------

#ifndef BOOST_FOREACH
#include <utility>  // for std::pair
#include <iterator> // for std::iterator_traits

#if !defined(_MSC_VER) & !defined(__cdecl)
#define __cdecl
#endif

namespace boost
{
namespace for_each
{

//-----------------------------------------------------------------------------
// yes_type, no_type
//-----------------------------------------------------------------------------
typedef char yes_type;
struct no_type { char dummy_[16]; };

//-----------------------------------------------------------------------------
// enable_if
//-----------------------------------------------------------------------------
template<bool>
struct enable_if_helper
{
    template<typename T> struct inner
    {
        typedef T type;
    };
};

template<>
struct enable_if_helper<false>
{
    template<typename T> struct inner
    {
    };
};

template<bool F,typename T>
struct enable_if
    : enable_if_helper<F>::template inner<T>
{
};

// HACKHACK: force VC7 to instantiate enable_if here
void dummy( enable_if<true,int> );
void dummy( enable_if<false,int> );

//-----------------------------------------------------------------------------
// static_any_t/static_any
//-----------------------------------------------------------------------------
struct static_any_base
{
    // static_any_base must evaluate to false in boolean context so that
    // they can be declared in if() statements.
    operator bool() const
    {
        return false;
    }
};

template<typename T>
struct static_any : static_any_base
{
    static_any( T const &t )
        : m_item( t )
    {
    }

    // temporaries of type static_any will be bound to const static_any_base
    // references, but we still want to be able to mutate the stored
    // data, so declare it as mutable.
    mutable T m_item;
};

typedef static_any_base const &static_any_t;

template<typename T>
inline T &static_any_cast( static_any_t a )
{
    return static_cast< static_any<T> const & >( a ).m_item;
}

//-----------------------------------------------------------------------------
// is_const
//-----------------------------------------------------------------------------
yes_type __cdecl check_is_const( void volatile const * );
no_type  __cdecl check_is_const( void volatile * );

template<typename T>
struct is_const
{
    enum { value = 1==sizeof(::boost::for_each::check_is_const(static_cast<T *>(0))) };
};

//-----------------------------------------------------------------------------
// is_char
//-----------------------------------------------------------------------------
yes_type __cdecl check_is_char( char const volatile * );
yes_type __cdecl check_is_char( wchar_t const volatile * );
no_type  __cdecl check_is_char( ... );

template<typename T>
struct is_char
{
    enum { value = 1==sizeof(::boost::for_each::check_is_char(static_cast<T *>(0))) };
};

//-----------------------------------------------------------------------------
// is_stl_container
//-----------------------------------------------------------------------------
template<typename T>
yes_type __cdecl check_is_stl_container( typename T::iterator *, typename T::const_iterator * );

template<typename T>
no_type  __cdecl check_is_stl_container( ... );

template<typename T>
struct is_stl_container
{
    enum { value = 1==sizeof(::boost::for_each::check_is_stl_container<T>(0,0)) };
};

//-----------------------------------------------------------------------------
// container_traits
//
//   for choosing between iterator/const_iterator based
//   on the const-ness of the container_traits.
//-----------------------------------------------------------------------------
template<int>
struct container_helper // non-stl containers.
{
    template<typename C> struct inner
    {
        typedef int iterator_type;  // dummy, not used
        typedef int reference_type; // dummy, not used
    };
};
template<> 
struct container_helper<1> // non-const stl containers
{
    template<typename C> struct inner
    {
        typedef typename C::iterator                iterator_type;
        typedef std::iterator_traits<iterator_type> traits_type;
        typedef typename traits_type::reference     reference_type;
    };
};
template<>
struct container_helper<2> // const stl containers
{
    template<typename C> struct inner
    {
        typedef typename C::const_iterator          iterator_type;
        typedef std::iterator_traits<iterator_type> traits_type;
        typedef typename traits_type::reference     reference_type;
    };
};

template<typename C>
struct container_traits
    : container_helper<is_stl_container<C>::value * ( is_const<C>::value + 1 )>::template inner<C>
{
};

//-----------------------------------------------------------------------------
// in_range
//
//   for making BOOST_FOREACH work with a pair of iterators.
//-----------------------------------------------------------------------------
template<typename T>
struct range_type;

template<typename T>
range_type<T> in_range( T &, T & );

template<typename T>
struct range_type
{
    T begin_;
    T end_;

    void operator&() const
    {
    }

private:
    friend range_type<T> in_range<T>( T &, T & );

    range_type( T begin, T end )
        : begin_( begin )
        , end_( end )
    {
    }
};

template<typename T>
inline range_type<T> in_range( T &begin, T &end )
{
    return range_type<T>( begin, end );
}

//
// leave this undefined
//
struct do_no_know_how_to_loop_over_type;

//-----------------------------------------------------------------------------
// native arrays
//-----------------------------------------------------------------------------
template< typename T, int N >
inline static_any<T *> begin( T (&rg)[N], int )
{
    return rg;
}

template< typename T, int N >
inline static_any<T *> end( T (&rg)[N], int )
{
    return rg + N;
}

template< typename T, int N >
inline bool done( static_any_t cur, static_any_t end, T (&)[N], int )
{
    return static_any_cast<T *>( cur ) == static_any_cast<T *>( end );
}

template< typename T, int N >
inline void next( static_any_t cur, T (&)[N], int )
{
    ++static_any_cast<T *>( cur );
}

template< typename T, int N >
inline T &extract( static_any_t cur, T (&)[N], int )
{
    return *static_any_cast<T *>( cur );
}

//-----------------------------------------------------------------------------
// null-terminated strings
//-----------------------------------------------------------------------------
template< typename Ch >
inline static_any<Ch *> begin( Ch *&sz, typename enable_if<is_char<Ch>::value,int>::type )
{
    return sz;
}

template< typename Ch >
inline static_any<int> end( Ch *&, typename enable_if<is_char<Ch>::value,int>::type )
{
    return 0; // not used
}

template< typename Ch >
inline bool done( static_any_t cur, static_any_t, Ch *&, typename enable_if<is_char<Ch>::value,int>::type )
{
    return ! *static_any_cast<Ch *>( cur );
}

template< typename Ch >
inline void next( static_any_t cur, Ch *&, typename enable_if<is_char<Ch>::value,int>::type )
{
    ++static_any_cast<Ch *>( cur );
}

template< typename Ch >
inline Ch &extract( static_any_t cur, Ch *&, typename enable_if<is_char<Ch>::value,int>::type )
{
    return *static_any_cast<Ch *>( cur );
}

//-----------------------------------------------------------------------------
// stl containers
//-----------------------------------------------------------------------------
template< typename T >
    inline static_any<typename container_traits<T>::iterator_type>
        begin( T &t, typename enable_if<is_stl_container<T>::value,int>::type )
{
    return t.begin();
}

template< typename T >
    inline static_any<typename container_traits<T>::iterator_type>
        end( T &t, typename enable_if<is_stl_container<T>::value,int>::type )
{
    return t.end();
}

template< typename T >
inline bool done( static_any_t cur, static_any_t end, T &, typename enable_if<is_stl_container<T>::value,int>::type )
{
    typedef typename container_traits<T>::iterator_type iter_t;
    return static_any_cast<iter_t>( cur ) == static_any_cast<iter_t>( end );
}

template< typename T >
inline void next( static_any_t cur, T &, typename enable_if<is_stl_container<T>::value,int>::type )
{
    typedef typename container_traits<T>::iterator_type iter_t;
    ++static_any_cast<iter_t>( cur );
}

template< typename T >
    inline typename container_traits<T>::reference_type
        extract( static_any_t cur, T &, typename enable_if<is_stl_container<T>::value,int>::type )
{
    typedef typename container_traits<T>::iterator_type iter_t;
    return *static_any_cast<iter_t>( cur );
}

//-----------------------------------------------------------------------------
// iterator ranges
//-----------------------------------------------------------------------------
template< typename T >
inline static_any<T> begin( range_type<T> const &rg, int )
{
    return rg.begin_;
}

template< typename T >
inline static_any<T> end( range_type<T> const &rg, int )
{
    return rg.end_;
}

template< typename T >
inline bool done( static_any_t cur, static_any_t end, range_type<T> const &, int )
{
    return static_any_cast<T>( cur ) == static_any_cast<T>( end );
}

template< typename T >
inline void next( static_any_t cur, range_type<T> const &, int )
{
    ++static_any_cast<T>( cur );
}

template< typename T >
inline typename std::iterator_traits<T>::reference extract( static_any_t cur, range_type<T> const &, int )
{
    return *static_any_cast<T>( cur );
}

//-----------------------------------------------------------------------------
// std::pair of iterators
//-----------------------------------------------------------------------------
template< typename T >
inline static_any<T> begin( std::pair<T, T> &rg, int )
{
    return rg.first;
}

template< typename T >
inline static_any<T> end( std::pair<T, T> &rg, int )
{
    return rg.second;
}

template< typename T >
inline bool done( static_any_t cur, static_any_t end, std::pair<T, T> &, int )
{
    return static_any_cast<T>( cur ) == static_any_cast<T>( end );
}

template< typename T >
inline void next( static_any_t cur, std::pair<T, T> &, int )
{
    ++static_any_cast<T>( cur );
}

template< typename T >
inline typename std::iterator_traits<T>::reference extract( static_any_t cur, std::pair<T, T> &, int )
{
    return *static_any_cast<T>( cur );
}

//-----------------------------------------------------------------------------
// unknown types
//-----------------------------------------------------------------------------
static_any<do_no_know_how_to_loop_over_type> __cdecl begin( ... );
static_any<int> __cdecl end( ... );
bool __cdecl done( ... );
void __cdecl next( ... );
int __cdecl extract( ... );

} // namespace for_each
} // namespace boost

//-----------------------------------------------------------------------------
// BOOST_FOREACH
//
//   For iterating over collections. Collections can be
//   arrays, null-terminated strings, or STL containers.
//   The loop variable can be a value or reference. For
//   example:
//
//   std::list<int> int_list( /*stuff*/ );
//   BOOST_FOREACH( int &i, int_list )
//   {
//       /* 
//        * loop body goes here.
//        * i is a reference to the int in int_list.
//        */
//   }
//
//   Alternately, you can declare the loop variable first,
//   so you can access it after the loop finishes. Obviously,
//   if you do it this way, then the loop variable cannot be
//   a reference.
//
//   int i;
//   BOOST_FOREACH( i, int_list )
//       { ... }
//-----------------------------------------------------------------------------
#define BOOST_FOREACH( VAR, COL )                                                                       \
    if       ( ::boost::for_each::static_any_t _for_each_cur = ::boost::for_each::begin( COL, 0 ) ) {}  \
    else if  ( ::boost::for_each::static_any_t _for_each_end = ::boost::for_each::end( COL, 0 ) ) {}    \
    else for ( bool _for_each_continue = true;                                                          \
               _for_each_continue && !::boost::for_each::done( _for_each_cur, _for_each_end, COL, 0 );  \
               _for_each_continue ? ::boost::for_each::next( _for_each_cur, COL, 0 ) : (void)&COL )     \
         if       ( _for_each_continue = false ) {}                                                     \
         else for ( VAR = ::boost::for_each::extract( _for_each_cur, COL, 0 );                          \
                   !_for_each_continue; _for_each_continue = true )


//-----------------------------------------------------------------------------
// Implementation notes:
//
//   1) The if/else blocks serve 2 purposes; (a) they allow
//      me to inject variables into subsequent scopes, and
//      (b) they ensure that BOOST_FOREACH expands into a single
//      statement.
//
//   2) The static_any_t type serves to decouple the
//      declared type of the local variables from their actual
//      type, which depends on the type of the collection
//      being iterated. Each function deduces the type of the
//      collection and uses that information to cast the
//      static_any_t type back to the correct concrete type. The
//      cast is static, and incurs no runtime overhead, but it
//      is perfectly safe.
//
//   3) The sole purpose of the nested for loop is to rebind
//      VAR at each iteration in case it is a reference. The
//      "for" statement is the only way to inject a variable
//      into a following scope without evaluating it in boolean
//      context.
//
//   4) The purpose of the _for_each_continue flag is twofold;
//      (a) it ensures that the nested for loop only executes
//      once for each iteration of the outer for loop, and
//      (b) it detects when a break statement has caused the
//      inner for loop to exit prematurely and relays that
//      information to the outer for loop.
//
//   5) The strange "(void)&COL" is to keep the code from
//      compiling if COL is not an lvalue. For instance, it should
//      not be a temporary, or a function call returning a
//      temporary. (Note: many popular compilers don't enforce
//      this, so beware!)
//
//   6) The routines for enumerating null-terminated strings take
//      the string by Ch*&. The "&" is needed to disabiguate when
//      enumerating a Ch[], which can match Ch(&)[] or Ch*.
//      However, a Ch[] cannot decay to a Ch*&, so Ch[] unambiguously
//      matches the Ch(&)[] overload. This also has the nice effect
//      of disallowing a Ch* that is not an lvalue, for compilers
//      that actually enforce that.
//
//   7) The function in_range() makes it possible to easily iterate
//      over an iterator range. in_range() returns a range_type, which
//      is taken by the various overloads by const reference, so it
//      does not have to be an lvalue. Also, range_type gets around
//      the restriction imposed by (5) above by defining an empty
//      operator&(). The in_range() function takes iterators by non-
//      const reference, so they must be lvalues themselves, and not
//      the return values of some function with side-effects.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Microsoft Specific
// 
// With Microsoft extensions enabled, temporaries can be bound
// to non-const references. That allows situations like below to
// compile, when they really shouldn't:
//
//   extern std::string get_string();
//   BOOST_FOREACH( char ch, get_string() ) { ... }
//
// The reason this is bad is that the BOOST_FOREACH loop will obtain
// iterators from a temporary string object and then access those
// iterators after the temporary has been destroyed. This results
// in access violations, data corruption and other fun stuff. (Aren't
// extensions wonderful?)
//
#if defined(_MSC_VER) & (_MSC_VER<1310) & defined(_MSC_EXTENSIONS)
#pragma message( "WARNING: use of BOOST_FOREACH is unsafe with Microsoft extensions enabled. Try compiling with /Za." )
#endif
//
//-----------------------------------------------------------------------------

#endif
