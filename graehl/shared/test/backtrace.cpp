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
#define DEBUG
#define MAIN

#include "debugprint.hpp"

void joe() {
    BACKTRACE;
    throw std::logic_error("something went wrong.");
}

void murphy() {
    BACKTRACE;
    joe();
}

int main()
{
    DBPC2("hi",1);
    try {
        BACKTRACE;
        murphy();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        BackTrace::print_on(std::cerr);
    }
}
