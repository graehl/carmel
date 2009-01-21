DIR=.
SHARED=$(DIR)/../shared
PROGS=mate

mate_LIB=$(BOOST_OPTIONS_LIB)
mate_NOTEST=1
mate_NOSTATIC=1
mate_SRC=mate.cpp

INC=. $(TRUNK) $(TRUNK)/sbmt_decoder/include $(TRUNK)/gusc
LIB=
CXX:=g++

CXXFLAGS_COMMON= -ggdb -ffast-math
CPPFLAGS += -DNO_BACKTRACE -DUSE_NONDET_RANDOM
CXXFLAGS += -O

default: all

include ../shared/graehl.mk
