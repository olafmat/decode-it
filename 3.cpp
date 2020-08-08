#include <iostream>
using namespace std;

int main() {
	int t;
	cin >> t;

    for (int i = 0; i < t; i++) {
        int n;
        cin >> n;
        cin.get();
        for (int k = 0; k < n; k++) {
            int a3 = cin.get() - 48,
                a2 = cin.get() - 48,
                a1 = cin.get() - 48,
                a0 = cin.get() - 48;
            cout << char((a3 + a2) * 10 + a1 + a0);
        }
        cout << endl;
    }

	return 0;
}
