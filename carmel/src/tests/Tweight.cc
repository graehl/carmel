#include "../weight.h"
using namespace std;
main()
{
  Weight a,b;
  for (;;) {
    cin >> a >> b;
	if (cin) {
	cout << Weight::out_ln << Weight::out_always_real << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	cout << Weight::out_always_log << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	cout << Weight::out_variable << "a=" << a << " b=" << b << " a*b=" << a*b << " a/b=" << a/b << " a+b=" << a+b << " a-b=" << a-b << endl;
	} else
		break;
  }
}
