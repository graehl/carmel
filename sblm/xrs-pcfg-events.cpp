# include <xrsparse/xrs.hpp>
//# include <xrsparse/xrs_grammar.hpp>
# include <string>
# include <iostream>
# include <sstream>

using namespace std;

int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);

    cout << print_rule_data_features(false);
    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
        } catch(...) { continue; }

        cout << rd << '\n';
    }
    return 0;
}
