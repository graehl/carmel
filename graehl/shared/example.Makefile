#gdb --args /cache/tt/bin/cygwin/forest-em.debug -f /cache/tt/sample/best_forest -I /cache/tt/sample/best_weights -n /cache/tt/sample/best_norm -i 1 -m 100k -w 3 -x sample/best_viterbi -L 9

PROGS= count-id-freq add-giza-models
#PROGS+=text-to-cc


count-id-freq_OBJ=count-id-freq.o
count-id-freq_SLIB=$(BOOST_OPT_LIB)
count-id-freq_NOTEST=1
#count-id-freq_NOSTATIC=1
#count-id-freq_NODEBUG=1

add-giza-models_OBJ=add-giza-models.o
add-giza-models_SLIB=$(BOOST_OPT_LIB)
add-giza-models_NOTEST=1


SHARED=../shared
INC= . $(SHARED)
LIB=
CXX:=g++

BASECXXFLAGS= -ggdb -ffast-math
CXXFLAGS= $(BASECXXFLAGS) -O -DNO_BACKTRACE -DUSE_NONDET_RANDOM
#-DSINGLE_PRECISION
## would have to link with boost random nondet source

CPPFLAGS_DEBUG+= -DDEBUG
CXXFLAGS_DEBUG= $(BASECXXFLAGS)
# -DDEBUGFIXEDINPUT
CPPFLAGS_TEST+= -DTEST -DDEBUG
CXXFLAGS_TEST=$(BASECXXFLAGS)
#CPP_EXT=cpp
ALL_CLEAN +=  *.restart.* *.swap.* *.stackdump *.d *.out *.log massif.* core

default: all
#forest-em-debug
#mydefault

vpath %.cpp .:$(SHARED)

include ../shared/graehl.mk


mydefault: $(BIN)/count-id-freq.debug
