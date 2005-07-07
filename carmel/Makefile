#*******************************************************************************
# This software ("Carmel") is licensed for research use only, as described in  *
# the LICENSE file in this distribution.  The software can be downloaded from  *
# http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
# contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
# with questions about the software or commercial licensing.  All software is  *
# copyrighted C 2000 by the University of Southern California.                 *
#*******************************************************************************

default: mydefault

DIR=.
SRC=$(DIR)/src
SHARED=$(DIR)/../shared
PROGS=carmel
carmel_OBJ=carmel.o fst.o train.o
vpath %.cc $(SRC):$(SHARED)
VPATH=$(SRC):$(SHARED)
INC=$(SRC) $(SHARED)
LIB = 
CPP_EXT=cc

#LDFLAGS = -lm
BASECXXFLAGS= -ffast-math
CXXFLAGS= $(BASECXXFLAGS) -O3 -s
CXXFLAGS_DEBUG= $(BASECXXFLAGS) -DDEBUG -ggdb
CXXFLAGS_TEST=$(BASECXXFLAGS) -DTEST

default: mydefault

include ../shared/graehl.mk

mydefault: all
	cd test && ./runtests.sh ../$(BIN)/carmel
#all

#CCFLAGS_PRF    = $(CCFLAGS) -O3 -pg
#LDFLAGS_PRF    = -pg
