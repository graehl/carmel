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
