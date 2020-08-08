#include <iostream>
#include <algorithm>
using namespace std;

int check(int a) {
    for (int n = 0;; n++) {
        if (a == 6174) {
            return n;
        }
        if (!a) {
            return -1;
        }

        int d[4];
        int m = a;
        for (int k = 0; k < 4; k++) {
            d[k] = m % 10;
            m /= 10;
        }

        sort(d, d + 4);

        int asc = ((d[0] * 10 + d[1]) * 10 + d[2]) * 10 + d[3];
        int desc = ((d[3] * 10 + d[2]) * 10 + d[1]) * 10 + d[0];
        a = desc - asc;
    }
}

int main() {
	int t;
	cin >> t;

    for (int i = 0; i < t; i++) {
        int a;
        cin >> a;
        cout << check(a) << endl;
    }

	return 0;
}
