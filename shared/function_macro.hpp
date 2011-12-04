#ifndef GRAEHL__SHARED__FUNCTION_MACRO_HPP
#define GRAEHL__SHARED__FUNCTION_MACRO_HPP

#define FUNCTION_OBJ_X(name,result,expr)        \
struct name \
{ \
    typedef result result_type; \
    template <class T1> \
    result_type operator()(const T1 &x) const \
    { \
        return expr; \
    } \
}

#define FUNCTION_OBJ_WRAP(funcname,result)         \
struct funcname ## _f \
{ \
    typedef result result_type; \
    template <class T1> \
    result_type operator()(const T1 &x) const \
    { \
        return funcname(x);                     \
    } \
}


#define FUNCTION_OBJ_X_Y(name,result,expr)        \
struct name \
{ \
    typedef result result_type; \
    template <class T1,class T2> \
    result_type operator()(const T1 &x,const T2 &y) const \
    { \
        return expr; \
    } \
}

#define PREDICATE_OBJ_X_Y(name,expr) FUNCTION_OBJ_X_Y(name,bool,expr)

namespace graehl {
PREDICATE_OBJ_X_Y(less_typeless,x<y);
PREDICATE_OBJ_X_Y(greater_typeless,x>y);
PREDICATE_OBJ_X_Y(equal_typeless,x==y);
PREDICATE_OBJ_X_Y(leq_typeless,x<=y);
PREDICATE_OBJ_X_Y(geq_typeless,x>=y);
PREDICATE_OBJ_X_Y(neq_typeless,x!=y);
}


#endif
