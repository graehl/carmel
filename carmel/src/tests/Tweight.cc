#include "../weight.h"
#include "../list.h"
#include <algorithm>
using namespace std;

main()
{
  List<Weight> l;
  Weight a,b;
  insert_iterator<List<Weight> > o(l,l.begin());
  for (;;) {
	  
    cin >> a >> b;
	if (cin) {
	*o++ = a;
	*o++ = b;
		Weight::out_ln(cout);Weight::out_always_real(cout); 
	cout << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	Weight::out_always_log(cout);
	cout << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	Weight::out_variable(cout);
	cout << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	} else
		break;
  }
  cout << "\n";
  for (List<Weight>::iterator i=l.begin();i!=l.end();++i)
	  cout << *i << " ";
  cout << "\n";
  cout << "\n";
  l.reverse();
  for (List<Weight>::const_iterator i=l.const_begin(),end=l.const_end();i!=end;++i)
	  cout << *i << " ";
  cout << "\n";

}
