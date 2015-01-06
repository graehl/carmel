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

#include "weight.h"

int main(int argc, char *argv[])
{
    Weight delta=1,sum=0,lastsum;
    Weight::default_never_log();
    double max=23456789,EPS=1e-7;
    unsigned period=1000000;
    std::cout.precision(12);
    unsigned quiet=0;
    double worstdiff=0,absworstdiff,totaldiff=0,totalabsdiff=0,i;
    Periodic p(period);
    std::cout << "sizeof(Weight)=" << sizeof(Weight) << "\n\n";
    
    for (i=0;i<max;lastsum=sum,sum+=delta,++i) {
        double diff=sum.getReal()-i;
        if (i>0 && lastsum == sum) {                
            std::cout << "Adding one didn't give any change from the last - " << sum << " after adding " << i<<" ones!\n";

            break;
        }
        totaldiff+=diff;
        totalabsdiff += fabs(diff);
        if (fabs(diff) > absworstdiff) {            
            worstdiff=diff;
            absworstdiff=fabs(diff);
//            std::cout << "\nNew worst difference between log and real accumulation: " << diff << " at i=" << i <<  "\n";
        }
        if (absdiff(sum,Weight(i)) > EPS)
            if (p.check())
                std::cout << "Log space addition of ones: " << sum << " differs from normal addition of ones: " << i << " by " << diff
                    //<< " or " << 100*(diff)/i << "%"
                          << "\n";
                
                
    }
    std::cout << "\nWorst difference between log and real accumulation: " << worstdiff << "\n";            
    std::cout << "\nAverage difference between log and real accumulation: " << totaldiff/i << "\n";            
    std::cout << "\nAverage absolute difference between log and real accumulation: " << totalabsdiff/i << "\n";            
    
    return 0;        
}
