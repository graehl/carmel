#ifndef MODELS_H 
#define MODELS_H 1
#include <vector>
#include <string>


// Define your first model
#define M1 "0 (0 (0 \"A\" \"A\" 0.75)  (0 \"AA\" \"A\" 0.25) (0 \"B\" \"B\" 0.67) (0 \"BB\" \"B\" 0.33))\0"

// Define your second model
#define M2 "0 (0 (0 \"A\" \"a\") (0 \"B\" \"b\")) \0"

// define additional models if neede and name them M3, M4, ...etc.


vector<string> Models ;
void initModels()
{
  // Note: order here is important 
  Models.push_back(M1); // add your first model to the list
  Models.push_back(M2); // add your second model to the list
  // Similarly, add additional models if necessary
}



#endif
