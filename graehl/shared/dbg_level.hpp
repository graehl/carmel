// Copyright 2014 Jonathan Graehl-http://graehl.org/
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

 .
*/

#ifndef GRAEHL_SHARED__DBG_LEVEL_HPP
#define GRAEHL_SHARED__DBG_LEVEL_HPP
#pragma once

#include <graehl/shared/os.hpp>

#define DECLARE_DBG_LEVEL_C(n, env) DECLARE_ENV_C_LEVEL(n, getenv_##env, env)
#define DECLARE_DBG_LEVEL(ch) DECLARE_DBG_LEVEL_C(ch##_DBG_LEVEL, ch##_DBG)
#define DECLARE_DBG_LEVEL_IF(ch) ch(DECLARE_DBG_LEVEL_C(ch##_DBG_LEVEL, ch##_DBG))

#endif
