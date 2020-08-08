#include <iostream>
using namespace std;

int main() {
	int n;

	cin >> n;

	for (int i = 0; i < n; i++) {
		int c, w, k;
		cin >> c >> w >> k;
		cout << (c * k <= w ? "yes" : "no") << endl;
	}

	return 0;
}		
