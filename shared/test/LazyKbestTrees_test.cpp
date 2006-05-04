//g++ -I/cache/shared -I/cache/boost test/LazyKBestTrees_test.cpp -o test/lkbtest.exe && test/lkbtest.exe 2>&1 | tee test/kbest.out && grep ^RESULT test/kbest.out > test/kbest.results

//g++ -ggdb -I/cache/shared -I/cache/boost test/LazyKBestTrees_test.cpp -o test/lkbtest.exe && test/lkbtest.exe 2>&1 | translate.pl -t test/ttable.node | tee test/kbest.out && grep ^RESULT test/kbest.out | tee test/kbest.results

#define GRAEHL__SINGLE_MAIN
#define INFO_LEVEL 999
#include "LazyKBestTrees.h"

int main(int argc,char *argv[])
{
    using namespace std;
    using namespace ns_TEST_lazy_kbest;
    //    cerr << "infolvl="<<ns_info_debug::debug.get_info_level()<<"\n";

    if (argc>1) {
        char c=argv[1][0];
        if (c=='1')
            simple_cycle();
        else if (c=='2')
            simplest_cycle();
        else
            jongraehl_example();
    } else
        jonmay_cycle();

    return 0;
}

