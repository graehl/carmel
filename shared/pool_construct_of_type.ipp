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
/** \file

    helpers for object construction - to include in your pool class - note lack
    of include guard.
*/

// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

// This file was AUTOMATICALLY GENERATED from "stdin"
//  Do NOT include directly!
//  Do NOT edit!

template <typename element_type, typename T0>
element_type * construct(T0 & a0)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0>
element_type * construct(const T0 & a0)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0>
element_type * construct(volatile T0 & a0)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0>
element_type * construct(const volatile T0 & a0)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(T0 & a0, T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const T0 & a0, T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(volatile T0 & a0, T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const volatile T0 & a0, T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(T0 & a0, const T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const T0 & a0, const T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(volatile T0 & a0, const T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const volatile T0 & a0, const T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(T0 & a0, volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const T0 & a0, volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(volatile T0 & a0, volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const volatile T0 & a0, volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(T0 & a0, const volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const T0 & a0, const volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(volatile T0 & a0, const volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1>
element_type * construct(const volatile T0 & a0, const volatile T1 & a1)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const volatile T1 & a1, T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const volatile T1 & a1, const T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const volatile T1 & a1, volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(T0 & a0, const volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const T0 & a0, const volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(volatile T0 & a0, const volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
template <typename element_type, typename T0, typename T1, typename T2>
element_type * construct(const volatile T0 & a0, const volatile T1 & a1, const volatile T2 & a2)
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(a0, a1, a2); }
  catch (...) { (free)(ret); throw; }
  return ret;


}
