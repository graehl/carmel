// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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

