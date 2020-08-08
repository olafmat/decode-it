#include <iostream>
using namespace std;

int main() {
	int r, s;
	cin >> r >> s;

    int a0 = s / r + 1;
    int a1 = a0 + 1;
    int b = s % r;

    int v = (1L << r);
    int i;
    for (i = 0; i < b; i++) {
        v *= a1;
    }
    for (; i < r; i++) {
        v *= a0;
    }

    cout << v << endl;
	return 0;
}
