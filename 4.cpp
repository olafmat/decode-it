#include <iostream>
#include <unordered_map>
#include <cstdlib>
#include <string>
using namespace std;

unordered_map<__uint32_t, __uint32_t> net;

inline __uint32_t readIP() {
    __uint32_t a0, a1, a2, a3;
    cin >> a0;
    cin.get();
    cin >> a1;
    cin.get();
    cin >> a2;
    cin.get();
    cin >> a3;
    return (a0 << 24) | (a1 << 16) | (a2 << 8) | a3;
}

inline __uint32_t root(__uint32_t node) {
    unordered_map<__uint32_t, __uint32_t>::iterator it, end = net.end();
    while (true) {
        it = net.find(node);
        if (it == end) {
            return node;
        }
        node = it -> second;
    }
}

void test() {
    for (int n = 1; n < 255; n++) {
        int m = (rand() % n) / 5 * 5 + (n % 5);
        if (rand() % 2) {
            cout << "B 1.1.1." << n << " 1.1.1." << m << endl;
        } else {
            cout << "B 1.1.1." << m << " 1.1.1." << n << endl;
        }
    }
    for (int n = 1; n < 255; n++) {
        cout << "T 1.1.1." << n << " 1.1.1.100" << endl;
    }
}

void test2() {
    for (int n = 1; n < 1000000; n++) {
        int a1 = (rand() % 255) + 1;
        int b1 = (rand() % 255) + 1;
        int a2 = (rand() % 255) + 1;
        int b2 = (rand() % 255) + 1;
        cout << (rand() % 2 ? "T " : "B ") << (rand() % 2 ? "  " : "")
            << (rand() % 2 ? "0" : "") << a1 << ".255.1." << b1 << (rand() % 2 ? "  " : " ")
            << (rand() % 2 ? "0" : "") << a2 << ".255.1." << b2 << (rand() % 2 ? "  " : "")
            << endl;
    }
}

int main() {
    //test();
    //test2();

    int eof = std::char_traits<char>::eof();
    string out;
    while (std::cin.peek() != eof) {
        char op = cin.get();
        if (op != 'T' && op != 'B') {
            continue;
        }
        __uint32_t ip1 = root(readIP());
        __uint32_t ip2 = root(readIP());

        if (op == 'B') {
            if (ip1 != ip2) {
                net[ip2] = ip1;
            }
        } else {
            out += (ip1 == ip2 ? "T\n" : "N\n");
        }
    }

    cout << out;
    return 0;
}
