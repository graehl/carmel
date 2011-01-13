TRUNK=../..
LOCAL=/home/nlg-02/graehl/isd/hpc-opteron
BOOST=$(LOCAL)/include
BOOSTLIB=$(LOCAL)/lib

vpath %.cpp .:$(TRUNK)/xrsparse/src

clean:
	rm -f *.o xrs-pcfg-events

xrs-pcfg-events: xrs.cpp xrs-pcfg-events.cpp
	g++ -o $@ $^ -I$(TRUNK)/xrsparse -I $(TRUNK)/gusc -I$(BOOST) -L$(BOOSTLIB) -lboost_thread -pthread
