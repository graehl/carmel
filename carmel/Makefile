#*******************************************************************************
# This software ("Carmel") is licensed for research use only, as described in  *
# the LICENSE file in this distribution.  The software can be downloaded from  *
# http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
# contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
# with questions about the software or commercial licensing.  All software is  *
# copyrighted C 2000 by the University of Southern California.                 *
#*******************************************************************************
# Change the following to the directory where you install carmel
DIR = .

# NOTE: some Linux machine assign i386 to the environment variable ARCH,
# so change i686 to i386 in in this file.
# Otherwise, you probably do not need to change anything beyond this line
# 
SRC = ${DIR}/src
SHARED = ${DIR}/../shared

vpath %.cc $(SRC):$(SHARED)

BOOST=/home/graehl/isd/boost

OBJ.linux = ${DIR}/obj/linux
OBJ.solaris = ${DIR}/obj/solaris
OBJ.cygwin = ${DIR}/obj/cygwin
OBJ.freebsd = ${DIR}/obj/freebsd
OBJ.macosx = ${DIR}/obj/macosx

OBJ = $(OBJ.${ARCH})
IN = -I${DIR}/src -I$(SHARED) -I- -I${BOOST} 
#-I${DIR}/STLport
BIN = ${DIR}/bin


CC.solaris = g++
#/nfs/topaz/graehl/isd/solaris/bin/g++3.2
CC.linux = g++
CC.cygwin = g++
CC.freebsd = g++
CC.macosx = g++

CC = $(CC.${ARCH})
CCFLAGS = -c -Wno-deprecated -DBOOST_DISABLE_THREADS -DBOOST_NO_MT ${INC}
#-Wall 
# -frepo
#-fno-exception
LDFLAGS = -L$(LIB) -lm
CCFLAGS_OPT    = $(CCFLAGS) -O3 -s -ffast-math
CCFLAGS_DBG    = $(CCFLAGS) -g -DDEBUG
CCFLAGS_TEST    = $(CCFLAGS) -DTEST -g -DDEBUG
CCFLAGS_PRF    = $(CCFLAGS) -O3 -pg
LDFLAGS_PRF    = -pg

BD = /home/graehl/isd/boost/libs/test/src
BOOST_TEST_SRC =  $(BD)/test_tools.cpp $(BD)/unit_test_parameters.cpp $(BD)/execution_monitor.cpp  $(BD)/unit_test_log.cpp $(BD)/unit_test_result.cpp $(BD)/supplied_log_formatters.cpp $(BD)/unit_test_main.cpp $(BD)/unit_test_suite.cpp $(BD)/unit_test_monitor.cpp

SRC_FILES_NO_MAIN = Arc.cc strhash.cc weight.cc wfstio.cc 2heap.cc 2hash.cc compose.cc fst.cc graph.cc kbest.cc train.cc 
SRC_FILES = $(SRC_FILES_NO_MAIN) finite.cc
SRC_MAIN = finite.cc
DEP_FILES = $(SRC_FILES:%.cc=%.d)
OBJ_FILES_OPT = ${SRC_FILES:%.cc=${OBJ}/optimized/%.o}
OBJ_LIB = ${SRC_FILES_NO_MAIN:%.cc=${OBJ}/optimized/%.o}
OBJ_FILES_PRF = ${SRC_FILES:%.cc=${OBJ}/profile/%.o}
OBJ_FILES_DBG = ${SRC_FILES:%.cc=${OBJ}/debug/%.o}
OBJ_FILES_PUR = ${SRC_FILES:%.cc=${OBJ}/purified/%.o}
OBJ_FILES_TEST = ${OBJ}/test/strhash.o 
#${SRC_FILES:%.cc=${OBJ}/test/%.o}
OBJ_OPT_DIR = $(OBJ)/optimized/
OBJ_PUR_DIR = $(OBJ)/purified/
OBJ_PRF_DIR = $(OBJ)/profiled/
OBJ_DBG_DIR = $(OBJ)/debug/
OBJ_TEST_DIR = $(OBJ)/test/

ifneq (${ARCH},macosx)
carmel: carmel.${ARCH} carmel.${ARCH}.dbg carmel.${ARCH}.static
else
carmel: carmel.${ARCH} carmel.${ARCH}.dbg
endif
carmel.${ARCH}:	$(OBJ_OPT_DIR) $(OBJ_FILES_OPT)
	$(CC) $(OBJ_FILES_OPT) $(LDFLAGS) -o $(BIN)/$@

carmel.${ARCH}.static:	$(OBJ_OPT_DIR) $(OBJ_FILES_OPT)
	$(CC) $(OBJ_FILES_OPT) $(LDFLAGS) --static -o $(BIN)/$@

debug: carmel.${ARCH}.dbg

carmel.${ARCH}.dbg:	 $(OBJ_DBG_DIR) $(OBJ_FILES_DBG)
	$(CC) $(OBJ_FILES_DBG) $(LDFLAGS) -o $(BIN)/$@
#--static

test: carmel.${ARCH}.test

pure: carmel.${ARCH}.pure

carmel.${ARCH}.test:	$(OBJ_TEST_DIR) $(OBJ_FILES_TEST)
	$(CC) $(BOOST_TEST_SRC} $(OBJ_FILES_TEST) $(LDFLAGS) -o $(BIN)/$@


carmel.${ARCH}.pure:	$(OBJ_PUR_DIR) $(OBJ_FILES_PUR)
	purify $(CC) $(OBJ_FILES_PUR) $(LDFLAGS) -o $(BIN)/$@

profile: carmel.${ARCH}.prf

carmel.${ARCH}.prf:  $(OBJ_PRF_DIR) $(OBJ_FILES_PRF) 
	$(CC) $(OBJ_FILES_PRF) $(LDFLAGS_PRF) -o $(BIN)/$@


carmel.a: $(OBJ_LIB)
	ar rv ${OBJ}/$@ $?

$(OBJ)/test/%.o: %.cc %.o
	$(CC)  $(CCFLAGS_TEST) $(IN) $< -o $@

$(OBJ)/optimized/%.o: %.cc %.o
	$(CC)  $(CCFLAGS_OPT) $(IN) $< -o $@

$(OBJ)/profile/%.o: %.cc %.o
	$(CC)  $(CCFLAGS_PRF) $(IN) $< -o $@

$(OBJ)/debug/%.o: %.cc %.o
	$(CC)  $(CCFLAGS_DBG) $(IN) $< -o $@

$(OBJ)/purified/%.o: %.cc %.o
	$(CC)  $(CCFLAGS_DBG) $(IN) $< -o $@


	$(CC)  $(CCFLAGS_DBG) $(IN) -I$(INC1) $< -o $@

$(OBJ)/test/:
	mkdir -p $@

$(OBJ)/optimized/:
	mkdir -p $@

$(OBJ)/profile/:
	mkdir -p $@

$(OBJ)/debug/:
	mkdir -p $@

$(OBJ)/purified/:
	mkdir -p $@

all: carmel.${ARCH}

clean:
	rm -rf $(OBJ)

cleanall:
	rm -rf $(OBJ) $(SRC)/*~


ifeq ($(MAKECMDGOALS),depend)
%.d: %.cc
	$(CC) -MM $< $(IN) | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@
endif

ifneq ($(MAKECMDGOALS),depend)
include $(DEP_FILES)
endif

depend: $(DEP_FILES)
#dddd	rm -rf $(DEP_FILES)
