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
