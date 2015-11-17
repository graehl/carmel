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
#include <iostream>

#define MAIN
//#define SINGLE_PRECISION
#define DOUBLE_PRECISION

#include <graehl/shared/tree.hpp>
#include <graehl/shared/string_to.hpp>

int main(int argc, char *argv[])
{
    using namespace graehl;
    using namespace std;
    
    if (argc<2) {
        cerr<<"argument: tree with int labels";
        return -1;
    }
    tree<int> t;
    std::string s(argv[1]);
    string_to(argv[1],t);
    cout << t << "\n";
    return 0;        
}
