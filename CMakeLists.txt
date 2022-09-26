set(USE_BOOST_ROOT "/local/graehl/c/xmt-externals/FC12/libraries/boost-1.69.0" CACHE STRING
  "If this exists, it's used instead of system path when finding boost libs")
option(USE_STATIC "prefer static linking except for libc" ON)
add_definitions(-fvisibility=hidden)
cmake_minimum_required(VERSION 3.15)
project(all)
if (USE_STATIC)
  set(ZLIB_USE_STATIC_LIBS ON)
  set(ZSTD_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_LIBS ON)
endif()
set(Boost_require_VERSION 1.53) # earlier is probably fine
set(CMAKE_CXX_STANDARD 17)
include_directories("${PROJECT_SOURCE_DIR}")
list(APPEND subdirs
  carmel
  forest-em
  )

macro(our_boost_libs)
  if (EXISTS "${USE_BOOST_ROOT}")
    set(Boost_NO_SYSTEM_PATHS 1)
    set(BOOST_ROOT "${USE_BOOST_ROOT}")
    set(BOOST_INCLUDEDIR "${USE_BOOST_ROOT}/include")
  endif()
  find_package(Boost "${Boost_require_VERSION}" COMPONENTS ${ARGV} REQUIRED)
  include_directories(${BOOST_INCLUDE_DIR})
  foreach(BLIB ${ARGV})
    list(APPEND OUR_BOOST_LIBS "Boost::${BLIB}")
  endforeach()
  message("boost libs: ${OUR_BOOST_LIBS}")
endmacro()

foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()
