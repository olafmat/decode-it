#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
using namespace std;

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "dirent.h"

struct Node {
    string name;
    int weight;
    unordered_set<Node*> edges;
    int freq;
};

unordered_set<Node*> nodes;
unordered_map<string, Node*> names;
unordered_set<Node*> dom;
unordered_set<Node*> ndom;

void removeNode(Node* node) {
    nodes.erase(node);
    ndom.erase(node);
    unordered_set<Node*>::iterator it;
    for (it = node->edges.begin(); it != node->edges.end(); it++) {
        Node* node2 = *it;
        node2->edges.erase(node);
    }
    node->edges.clear();
}

bool isDominated(Node *node, Node* excl) {
    if (node != excl && dom.count(node)) {
        return true;
    }
    for (unordered_set<Node*>::iterator it = node->edges.begin(); it != node->edges.end(); it++) {
        Node* node2 = *it;
        if (node2 != excl && dom.count(node2)) {
            return true;
        }
    }
    return false;
}

double score(Node *node) {
    int sum = 0;
    if (!isDominated(node, node)) {
        sum += node->freq;
    }
    unordered_set<Node*>::iterator it;
    for (it = node->edges.begin(); it != node->edges.end(); it++) {
        Node* node2 = *it;
        if (!isDominated(node2, node)) {
            sum += node2->freq;
        }
    }
    return double(sum) / node->weight;
}


void loadData() {
	int t;
	cin >> t;

    for (int i = 0; i < t; i++) {
        Node * node = new Node();
        cin >> node -> name >> node -> weight;
        node->freq = 1;
        nodes.insert(node);
        ndom.insert(node);
        names[node->name] = node;
    }

    int m;
    cin >> m;
    for (int j = 0; j < m; j++) {
        string name1, name2;
        cin >> name1 >> name2;
        Node *node1 = names[name1];
        Node *node2 = names[name2];
        node1->edges.insert(node2);
        node2->edges.insert(node1);
    }
}

void bruteForce() {
    uint32_t max = uint32_t(1) << ndom.size();
    unordered_set<Node*> bestDom;
    int32_t bestCost = 0x7FFFFFFF;
    for (uint32_t iter = 1; iter < max; iter++) {
        int cost = 0;
        uint32_t c = iter;
        dom.clear();
        for (unordered_set<Node*>::iterator it = nodes.begin(); c != 0 && it != nodes.end(); it++, c >>= 1) {
            if (c & 1) {
                Node *node = *it;
                dom.insert(node);
                cost += node->weight;
            }
        }
        if (cost > bestCost) {
            continue;
        }

        bool ok = true;
        for (unordered_set<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++) {
            if (!isDominated(*it, NULL)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            bestCost = cost;
            bestDom = dom;
        }
    }
    dom = bestDom;
}

void findDominatingSet() {
/*    bool change = true;
//cout << __LINE__ <<endl;
    while(change) {
        change = false;
//cout << __LINE__ <<endl;
        for (unordered_set<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++) {
//cout << __LINE__ <<endl;
            Node* node = *it;
//cout << __LINE__ <<endl;
            int size = node->edges.size();
//cout << __LINE__ <<endl;
            if (size == 0) {
//cout << __LINE__ <<endl;
                removeNode(node);
//cout << __LINE__ <<endl;
                dom.insert(node);
//cout << __LINE__ <<endl;
                ndom.erase(node);
//cout << __LINE__ <<endl;
                change = true;
            };

//cout << __LINE__ <<endl;
            int total = 0;
//cout << __LINE__ <<endl;
            for (unordered_set<Node*>::iterator it2 = node->edges.begin(); it2 != node->edges.end(); it2++) {
                Node *node2 = *it2;
//cout << __LINE__ <<endl;
                if (node2->edges.size() == 1) {
//cout << __LINE__ <<endl;
                    total += node2->weight;
                }
            };
//cout << __LINE__ <<endl;
            if (total > node->weight) {
//cout << __LINE__ <<endl;
                for (unordered_set<Node*>::iterator it2 = node->edges.begin(); it2 != node->edges.end(); it2++) {
//cout << __LINE__ <<endl;
                    Node *node2 = *it2;
//cout << __LINE__ <<endl;
                    if (node2->edges.size() == 1) {
//cout << __LINE__ <<endl;
                        removeNode(node2);
//cout << __LINE__ <<endl;
                    }
                }
//cout << __LINE__ <<endl;
                removeNode(node);
//cout << __LINE__ <<endl;
                dom.insert(node);
//cout << __LINE__ <<endl;
                ndom.erase(node);
//cout << __LINE__ <<endl;
                change =true;
            }
        }
    }
*/
    bool repeat = !ndom.empty();
    while(repeat) {
        double bestScore = 0;
        Node *best;
        for (unordered_set<Node*>::iterator it = ndom.begin(); it != ndom.end(); it++) {
            Node *node = *it;
            double sc = score(node);
            if (sc > bestScore) {
                bestScore = sc;
                best = node;
            }
        }
        if (bestScore == 0) {
            break;
        }
        dom.insert(best);
        unordered_set<Node*> nndom;

        repeat = false;
        for (unordered_set<Node*>::iterator it = ndom.begin(); it != ndom.end(); it++) {
            Node *node = *it;
            if (!isDominated(node, NULL)) {
                repeat = true;
                nndom.insert(node);
                node->freq++;
            }
        }
        ndom = nndom;
    }

    unordered_set<Node*> ndom = dom;
    for (unordered_set<Node*>::iterator it = ndom.begin(); it != ndom.end(); it++) {
        Node *node = *it;
        if (score(node) == 0) {
            dom.erase(node);
        }
    }
}

void printResults() {
    cout << dom.size() << endl;
    int total = 0;
    for (unordered_set<Node*>::iterator it = dom.begin(); it != dom.end(); it++) {
        Node *node = *it;
        cout << node->name << endl;
        total += node->weight;
    }
    cout << total << endl;
}

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main() {
    signal(SIGSEGV, handler);
    loadData();
    if (nodes.size() <= 20) {
        bruteForce();
    } else {
        findDominatingSet();
    }
    printResults();
	return 0;
}
