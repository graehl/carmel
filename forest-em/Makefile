PROGS= forest-em forestviz

forest-em_SRC=forest-em.cpp
forest-em_LIB=$(BOOST_OPTIONS_LIB)
forest-em_NOTEST=1

text-to-cc_SRC=text-to-cc.cpp
text-to-cc_NOTEST=1
text-to-cc_NOSTATIC=1
text-to-cc_NODEBUG=1

forestviz_SRC=forestviz.cpp
forestviz_LIB=$(BOOST_OPTIONS_LIB)
forestviz_NOTEST=1
#forestviz_NOSTATIC=1

INC= . $(TRUNK)
LIB=
CXX:=g++

CXXFLAGS_COMMON= -ggdb -ffast-math
CPPFLAGS += -DNO_BACKTRACE -DUSE_NONDET_RANDOM
CXXFLAGS += -O
#-DSINGLE_PRECISION
## would have to link with boost random nondet source
#
CPPFLAGS_DEBUG+= -DDEBUG
# -DDEBUGFIXEDINPUT
CPPFLAGS_TEST+= -DTEST -DDEBUG

#CPP_EXT=cpp
ALL_CLEAN += forest-em.README.hpp *.restart.* *.swap.* *.stackdump *.d *.out *.log massif.* core

default: all
include ../shared/graehl.mk
vpath %.cpp .:$(SHARED)

forest-em.README.hpp: forest-em.README $(BIN)/text-to-cc
	$(BIN)/text-to-cc usage_str < $< > $@

depend: forest-em.README.hpp

forestviz-test: $(BIN)/forestviz
	$< -n -i sample/forests -o sample/forests.dot
	dot -Tps sample/forests.dot -o sample/forests.ps

forest-regress=test/testregress.sh
forest-em-regress: $(BIN)/forest-em
	$(forest-regress)

forest_input=sample/forests
norm_input=sample/norm
derivs_dir=sample/derivs
derivs_n=10
iter=5
forest-em-sample: $(BIN)/forest-em
#	$< -f $(forest_input) -L 0 -R sample/rule_list -w 3 -W 10 -D 5 -r 0 -e 1e-5 -i 2000
	$< -f $(derivs_dir)/first$(derivs_n).deriv -n $(derivs_dir)/first$(derivs_n).norm --rules-file $(derivs_dir)/first$(derivs_n).rules  --watch-rule 2359 --watch-depth 7 --watch-period 1 -o /dev/null -T 100 --max-iter $(iter) -e .001 -X .1

forest-em-gibbs: $(BIN)/forest-em.debug
	$< -f sample/derivs/first$(derivs_n).deriv -n sample/derivs/first$(derivs_n).norm --crp=100 --outsample-file=sample/derivs/first$(derivs_n).sample  --print-counts-to=0 --print-counts-sparse=1 -o sample/derivs/first$(derivs_n).prob -v sample/derivs/first$(derivs_n).viterbi
#	$< -f $(derivs_dir)/first$(derivs_n).deriv -n $(derivs_dir)/first$(derivs_n).norm --rules-file $(derivs_dir)/first$(derivs_n).rules  --watch-rule 2359 --watch-depth 7 --watch-period 1 -o /dev/null -T 100 --max-iter $(iter) -e .001 -X .1 --crp=1000 --alpha=.1

forest-em-debug: $(BIN)/forest-em.debug $(forest_input) $(norm_input)
	$< --help
#	$< -f sample/best_forest -n sample/best_norm -i 1 -m 100k -W 1 -w 1 -x sample/checkpoint -L 2 -V 1 -v - -o - -u
#	$< -f sample/best_forest -n sample/best_norm -m 10k -R sample/rule_list -w 3 -W 1 -D 3 -r 0 -e 1e-2 -i 20 -X 0.1 -Y .1 -x sample/best_viterbi -L 4
#	$< -f $(forest_input) -n $(norm_input) -m 10k -R sample/rule_list -w 3 -W 1 -D 3 -r 0 -e 1e-2 -i 20 -b sample/byid_rules -B - -X 0.1 -x sample/sample_viterbi -F '' -Y .001 -u -H
#	$< -f $(forest_input) -n $(norm_input)
#	$< -f $(derivs_dir)/first$(derivs_n).deriv -n $(derivs_dir)/first$(derivs_n).norm --rules-file $(derivs_dir)/first$(derivs_n).rules  --watch-rule 2359 --watch-depth 20 --watch-period 1 -o /dev/null -T 100 -m 10000 -e .001 -L 0
#	$< -f $(forest_input) -n sample/empty.norm -m 10k -R sample/rule_list -w 3 -W 1 -D 3 -r 0 -e 1e-2 -i 20 -b sample/byid_rules -B - -X 0.1 -x sample/sample_viterbi -F '' -Y .001 -u -H
