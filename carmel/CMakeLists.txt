project(carmel)
our_boost_libs(random timer)
add_executable(carmel src/carmel.cc src/fst.cc src/train.cc src/gibbs.cc)
target_link_libraries(carmel ${OUR_BOOST_LIBS})
