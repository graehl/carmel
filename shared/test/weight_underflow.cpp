#include <iostream>

#define MAIN
#define SINGLE_PRECISION

#include "weight.h"

int main(int argc, char *argv[])
{
    Weight delta=1,sum=0,lastsum;
    double max=123456789;
    for (double i=0;i<max;++i) {
        lastsum=sum;
        sum += delta;
        if (sum < i) {
            std::cerr << "Warning: log space addition of ones: " << sum << " is less than normal addition of ones: " << i << std::endl;
            if (lastsum == sum) {                
                std::cerr << " and further, adding one didn't give any change from the last!\n";
                return 1;            
            }            
        }
    }
    return 0;        
}
