#*******************************************************************************
# This software ("Carmel") is licensed for research use only, as described in  *
# the LICENSE file in this distribution.  The software can be downloaded from  *
# http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
# contact Jonathan Graehl (graehl@gmail.com) or Kevin Knight (knight@isi.edu)  *
# with questions about the software or commercial licensing.  All software is  *
# copyrighted C 2000 by the University of Southern California.                 *
#*******************************************************************************

default: mydefault

DIR=.
SRC=$(DIR)/src
CP_PROGS=$(wildcard $(SRC)/*.pl)
GRAEHL ?= $(DIR)/..
OPENFSTSRC=$(firstword $(wildcard $(GRAEHL)/openfst*/src) $(wildcard $(GRAEHL)/../openfst*/src))
SHARED=$(GRAEHL)/shared
PROGS=carmel
carmel_SRC=carmel.cc fst.cc train.cc gibbs.cc
carmel_NOTEST=1
carmel_LIB=$(BOOST_RANDOM_LIB) $(BOOST_TIMER_LIB)
vpath %.cc $(SRC):$(SHARED)
VPATH=$(SRC):$(SHARED)
DEFS=
INC=$(SRC) $(SHARED)
LIB =
CPP_EXT=cc

LDFLAGS += -lpthread -ldl -pthread
BASECXXFLAGS= -ffast-math
CPPFLAGS += -Wno-deprecated -Wall
ifeq ($(HOST),cage)
CXXFLAGS= $(BASECXXFLAGS) -O
LDFLAGS += -lstdc++
# -pg
else
CXXFLAGS= $(BASECXXFLAGS) -O3
endif
#FIXME: crashes with -O3 on kbest output
CXXFLAGS_DEBUG= $(BASECXXFLAGS) -DDEBUG -ggdb
CXXFLAGS_TEST=$(BASECXXFLAGS) -DTEST

ifeq ($(OPENFSTSRC),)
# openfst sources not found - don't enable
else
INC+=$(OPENFSTSRC)/include $(OPENFSTSRC)
#DEFS+=USE_OPENFST

#add USE_OPENFST def to enable openfst (note: we attempt to build the appropriate sources
#directly, which might break with newer openfst versions - see OPENFSTSRC)
endif

default: mydefault

include ../shared/graehl.mk

mydefault: all

debug: $(BIN)/carmel.debug

tests: $(BIN)/carmel.debug
	cd test && ./runtests.sh ../$<
#all

#CCFLAGS_PRF    = $(CCFLAGS) -O3 -pg
#LDFLAGS_PRF    = -pg
