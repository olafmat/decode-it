#include <iostream>
#include <unordered_map>
using namespace std;

unordered_map<unsigned, unsigned> net;

inline unsigned readIP() {
    unsigned a0, a1, a2, a3;
    cin.get();
    cin >> a0;
    cin.get();
    cin >> a1;
    cin.get();
    cin >> a2;
    cin.get();
    cin >> a3;
    return (a0 << 24) | (a1 << 16) | (a2 << 8) | a3;
}

inline unsigned root(unsigned node) {
    unordered_map<unsigned, unsigned>::iterator it, end = net.end();
    while (true) {
        it = net.find(node);
        if (it == end) {
            return node;
        }
        node = it -> second;
    }
}

int main() {
    int eof = std::char_traits<char>::eof();
    while (std::cin.peek() != eof) {
        char op = cin.get();
        if (op != 'T' && op != 'B') {
            break;
        }
        unsigned ip1 = root(readIP());
        unsigned ip2 = root(readIP());
        cin.get();

        if (op == 'B') {
            if (ip1 != ip2) {
                net[ip1] = ip2;
            }
        } else {
            cout << (ip1 == ip2 ? 'T' : 'N') << endl;
        }
    }

    return 0;
}
