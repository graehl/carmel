#ifndef MODELS_H 
#define MODELS_H 1
#include <vector>
#include <string>

char *ModelsDef[] = {
  "0 (0 (0 \"A\" \"A\" 0.75)  (0 \"AA\" \"A\" 0.25) (0 \"B\" \"B\" 0.67) (0 \"BB\" \"B\" 0.33))",
  "0 (0 (0 \"A\" \"a\") (0 \"B\" \"b\"))"
};
// define additional models if needed


vector<string> Models ;
void initModels()
{
  // Similarly, add additional models if necessary
  int n_models = sizeof(ModelsDef)/sizeof(char *);
  for (int i=0;i<n_models;++i)
    Models.push_back(ModelsDef[i]);
}



#endif
