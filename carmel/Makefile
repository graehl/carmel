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
SHARED = ${DIR}/../shared
SRC = ${DIR}/src
OBJ.i686 = ${DIR}/obj/i686
OBJ.sun4 = ${DIR}/obj/sun4
OBJ.linux = ${DIR}/obj/linux
OBJ.freebsd = ${DIR}/obj/freebsd
OBJ.macosx = ${DIR}/obj/macosx
INC = ${DIR}/src
#-I${DIR}/STLport
BIN = ${DIR}/bin
CC.sun4 = g++
CC.i686 = g++
CC.cygwin = g++
CC = $(CC.$(ARCH))
CCFLAGS = -c -Wall
LDFLAGS = -L$(LIB) -lm
CCFLAGS_OPT    = $(CCFLAGS) -O3 -ffast-math
CCFLAGS_DBG    = $(CCFLAGS) -g 
CCFLAGS_TEST    = $(CCFLAGS) -DTEST -g -DDEBUG
CCFLAGS_PRF    = $(CCFLAGS) -O3 -pg
LDFLAGS_PRF    = -pg
BOOST_TEST_SRC =  $(BD)/test_tools.cpp $(BD)/unit_test_parameters.cpp $(BD)/execution_monitor.cpp  $(BD)/unit_test_log.cpp $(BD)/unit_test_result.cpp $(BD)/supplied_log_formatters.cpp $(BD)/unit_test_main.cpp $(BD)/unit_test_suite.cpp $(BD)/unit_test_monitor.cpp
SRC_FILES_NO_MAIN = $(SRC)/Arc.cc $(SRC)/strhash.cc $(SRC)/weight.cc $(SRC)/wfstio.cc $(SRC)/2heap.cc $(SRC)/2hash.cc $(SRC)/compose.cc $(SRC)/fst.cc $(SRC)/graph.cc $(SRC)/kbest.cc $(SRC)/train.cc 
SRC_FILES_NO_MAIN = $(SRC)/Arc.cc $(SRC)/strhash.cc $(SRC)/weight.cc $(SRC)/wfstio.cc $(SRC)/2heap.cc $(SRC)/2hash.cc $(SRC)/compose.cc $(SRC)/fst.cc $(SRC)/graph.cc $(SRC)/kbest.cc $(SRC)/train.cc 
SRC_FILES = $(SRC_FILES_NO_MAIN) $(SRC)/finite.cc
SRC_MAIN = $(SRC)/finite.cc

OBJ_FILES_OPT = ${SRC_FILES:${SRC}/%.cc=${OBJ}/optimized/%.o}
OBJ_LIB = ${SRC_FILES_NO_MAIN:${SRC}/%.cc=${OBJ}/optimized/%.o}
OBJ_FILES_PRF = ${SRC_FILES:${SRC}/%.cc=${OBJ}/profile/%.o}
OBJ_FILES_DBG = ${SRC_FILES:${SRC}/%.cc=${OBJ}/debug/%.o}
#${SRC_FILES:${SRC}/%.cc=${OBJ}/test/%.o}
OBJ_OPT_DIR = $(OBJ)/optimized/
OBJ_PUR_DIR = $(OBJ)/purified/
OBJ_PRF_DIR = $(OBJ)/profiled/
OBJ_TEST_DIR = $(OBJ)/test/
carmel: carmel.${ARCH}
endif
carmel.${ARCH}:	$(OBJ_OPT_DIR) $(OBJ_FILES_OPT)
	$(CC) $(OBJ_FILES_OPT) $(LDFLAGS) --static -o $(BIN)/$@

debug: carmel.${ARCH}.dbg

	$(CC) $(OBJ_FILES_DBG) $(LDFLAGS) -o $(BIN)/$@
#--static

pure: carmel.${ARCH}.pure

carmel.${ARCH}.pure:	$(OBJ_PUR_DIR) $(OBJ_FILES_PUR)
	purify $(CC) $(OBJ_FILES_PUR) $(LDFLAGS) -o $(BIN)/$@

profile: carmel.${ARCH}.prf

carmel.${ARCH}.prf:  $(OBJ_PRF_DIR) $(OBJ_FILES_PRF) 
	$(CC) $(OBJ_FILES_PRF) $(LDFLAGS_PRF) -o $(BIN)/$@


carmel.a: $(OBJ_LIB)
	ar rv ${OBJ}/$@ $?

	$(CC)  $(CCFLAGS_OPT) -I$(INC) $< -o $@
	$(CC)  $(CCFLAGS_OPT) $(IN) $< -o $@

	$(CC)  $(CCFLAGS_PRF) -I$(INC) $< -o $@
	$(CC)  $(CCFLAGS_PRF) $(IN) $< -o $@

	$(CC)  $(CCFLAGS_DBG) -I$(INC) $< -o $@
	$(CC)  $(CCFLAGS_DBG) $(IN) $< -o $@

	$(CC)  $(CCFLAGS_DBG) -I$(INC) $< -o $@
	$(CC)  $(CCFLAGS_DBG) $(IN) $< -o $@

	$(CC)  $(CCFLAGS_DBG) -I$(INC) -I$(INC1) $< -o $@
	$(CC)  $(CCFLAGS_DBG) $(IN) -I$(INC1) $< -o $@

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
depend: 
	rm -rf $(DIR)/dependencies.${ARCH}
	$(CC) -MM $(SRC)/*.cc -I$(INC) | perl -e 'while(<>){s?^([^\:]+\.o:)?$(OBJ)\/optimized\/\1?g;print;}' > $(DIR)/dependencies.${ARCH}
	$(CC) -MM $(SRC)/*.cc -I$(INC) | perl -e 'while(<>){s?^([^\:]+\.o:)?$(OBJ)\/profile\/\1?g;print;}' >> $(DIR)/dependencies.${ARCH}
	$(CC) -MM $(SRC)/*.cc -I$(INC) | perl -e 'while(<>){s?^([^\:]+\.o:)?$(OBJ)\/debug\/\1?g;print;}' >> $(DIR)/dependencies.${ARCH}
	$(CC) -MM $(SRC)/*.cc -I$(INC) | perl -e 'while(<>){s?^([^\:]+\.o:)?$(OBJ)\/purified\/\1?g;print;}' >> $(DIR)/dependencies.${ARCH}
	rm -rf $(OBJ) $(SRC)/*~
include $(DIR)/dependencies.${ARCH}
DEPS=$(DIR)/dependencies.${ARCH}
include $(DEPS)
