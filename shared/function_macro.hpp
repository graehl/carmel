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
#ifndef GRAEHL__SHARED__FUNCTION_MACRO_HPP
#define GRAEHL__SHARED__FUNCTION_MACRO_HPP
#pragma once

#define FUNCTION_OBJ_X(name, result, expr)        \
struct name \
{ \
    typedef result result_type; \
    template <class T1> \
    result_type operator()(T1 const& x) const \
    { \
        return expr; \
    } \
}

#define FUNCTION_OBJ_WRAP(funcname, result)         \
struct funcname ## _f \
{ \
    typedef result result_type; \
    template <class T1> \
    result_type operator()(T1 const& x) const \
    { \
        return funcname(x);                     \
    } \
}


#define FUNCTION_OBJ_X_Y(name, result, expr)        \
struct name \
{ \
    typedef result result_type; \
    template <class T1, class T2> \
    result_type operator()(T1 const& x, T2 const& y) const \
    { \
        return expr; \
    } \
}

#define PREDICATE_OBJ_X_Y(name, expr) FUNCTION_OBJ_X_Y(name, bool, expr)

namespace graehl {
PREDICATE_OBJ_X_Y(less_typeless, x<y);
PREDICATE_OBJ_X_Y(greater_typeless, x>y);
PREDICATE_OBJ_X_Y(equal_typeless, x==y);
PREDICATE_OBJ_X_Y(leq_typeless, x<=y);
PREDICATE_OBJ_X_Y(geq_typeless, x>=y);
PREDICATE_OBJ_X_Y(neq_typeless, x!=y);


}

#endif
