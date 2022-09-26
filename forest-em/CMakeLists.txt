project(forest-em)
our_boost_libs(program_options serialization system filesystem
  random chrono timer iostreams filesystem
  unit_test_framework
  )
find_package(zstd QUIET)
message("zstd found?: ${zstd_FOUND}")
set(zstdlib)
if (zstd_FOUND)
  # at least with mac+brew boost, iostreams appears to pull in zstd, lzma, z, bz2 libs; -lzstd wasn't found
  link_directories("/usr/local/lib")
  list(APPEND zstdlib zstd::libzstd_static)
endif()
add_executable(text-to-cc ../graehl/shared/text-to-cc.cpp)
include_directories(${PROJECT_SOURCE_DIR})
set(README_IN forest-em.README)
set(README_GEN_H ${PROJECT_SOURCE_DIR}/forest-em.README.hpp)
add_custom_command(OUTPUT ${README_GEN_H}
  INPUT ${README_IN}
  COMMAND text-to-cc usage_str < ${PROJECT_SOURCE_DIR}/${README_IN} > ${README_GEN_H}
  VERBATIM
  )
add_executable(forest-em forest-em.cpp)
set_property(SOURCE forest-em.cpp APPEND PROPERTY OBJECT_DEPENDS ${README_GEN_H})
set_property(TARGET forest-em APPEND PROPERTY OBJECT_DEPENDS ${README_GEN_H})
add_executable(forestviz forestviz.cpp)

target_link_libraries(forest-em Boost::timer Boost::random Boost::iostreams Boost::program_options ${zstdlib})
target_link_libraries(forestviz Boost::random Boost::iostreams Boost::program_options ${zstdlib})
