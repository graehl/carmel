STD = -std=c++11
STDLIB = -stdlib=libc++
CXXFLAGS = $(STD) $(STDLIB)
balance: balance.lex.cc balance.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^
%.lex.cc %.lex.hh: %.lext
	flex --header-file=$*.lex.hh --outfile=$*.lex.cc $<
