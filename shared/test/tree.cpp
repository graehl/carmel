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
