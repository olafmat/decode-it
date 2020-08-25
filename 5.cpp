#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include "dirent.h"

using namespace std;


//#define VERIFY

#define MAX_NODES 300
#define MAX_CHUNKS ((MAX_NODES + 63) / 64)

struct Node {
    int idA;
    uint64_t idU, idNU;
    string name;
    int weight;
    int nedges;
    Node* edges[MAX_NODES];
    int freq;
    int& score;
    //int conf;

    Node(int& score): score(score) {
    }
};

int nnodes;
Node* nodeArr[MAX_NODES];
int scores[MAX_NODES];

class NodeSet {
    uint64_t bset[MAX_CHUNKS];

public:
    void clear() {
        memset(bset, 0, sizeof(bset));
    }

    NodeSet() {
        clear();
    }

    void setAll() {
        int last = nnodes >> 6;
        for (int a = 0; a < last; a++) {
            bset[a] = int64_t(-1);
        }
        bset[last] = (uint64_t(1) << (nnodes & 63)) - 1;
    }

    inline void insert(const Node* node) {
        bset[node->idA] |= node->idU;
    }

    inline void erase(const Node* node) {
        bset[node->idA] &= node->idNU;
    }

    inline bool count(const Node* node) const {
        return (bset[node->idA] & node->idU) != 0;
    }

    int size() const {
        int len = 0;
        for (int a = 0; a < MAX_CHUNKS; a++) {
            uint64_t u = bset[a];
            while (u != uint64_t(0)) {
                len += (u & 1);
                u >>= 1;
            }
        }
        return len;
    }

    bool empty() const {
        for (int a = 0; a < MAX_CHUNKS; a++) {
            if (bset[a] != uint64_t(0)) {
                return false;
            }
        }
        return true;
    }

    inline void operator -=(NodeSet& b) {
        for (int a = 0; a < MAX_CHUNKS; a++) {
            bset[a] &=~ b.bset[a];
        }
    }

    inline void operator +=(NodeSet& b) {
        for (int a = 0; a < MAX_CHUNKS; a++) {
            bset[a] |= b.bset[a];
        }
    }

    NodeSet& operator=(NodeSet &src) {
        memcpy(bset, src.bset, sizeof(bset));
        return *this;
    }

    struct iterator {
        uint64_t* bset;
        int a;
        uint64_t b;
        int bit;

        void operator++(int) {
            if (a >= MAX_CHUNKS) {
                return;
            }
            while(true) {
                b <<= 1;
                bit++;
                if (b == uint64_t(0) || b > bset[a]) {
                    b = 1;
                    bit = 0;
                    a++;
                    if (a >= MAX_CHUNKS) {
                        return;
                    }
                }
                if ((bset[a] & b) != uint64_t(0)) {
                    return;
                }
            }
        }

        inline bool operator != (int x) {
            return a < MAX_CHUNKS;
        }

        inline Node* operator*() const {
            //cout << this << " " << a << " " << bit << " " << nodeArr[(a << 6) + bit] <<endl;
            return nodeArr[(a << 6) + bit];
        }
    };

    inline iterator begin() {
        iterator it;
        it.bset = bset;
        it.a = -1;
        it.b = 0;
        it.bit = -1;
        it++;
        return it;
    }

    struct niterator {
        uint64_t* bset;
        int a;
        uint64_t b;
        int n;

        void operator++(int) {
            if (n >= nnodes) {
                return;
            }
            while(true) {
                b <<= 1;
                n++;
                if (n >= nnodes) {
                    return;
                }
                if (!(n & 63)) {
                    b = 1;
                    a++;
                }
                if ((bset[a] & b) == uint64_t(0)) {
                    return;
                }
            }
        }

        inline bool operator != (int x) {
            return n < nnodes;
        }

        inline Node* operator*() const {
            //cout << this << " " << a << " " << n << " " << nnodes << " " << nodeArr[n] <<endl;
            return nodeArr[n];
        }
    };

    inline niterator nbegin() {
        niterator it;
        it.bset = bset;
        it.a = -1;
        it.b = 0;
        it.n = -1;
        it++;
        return it;
    }

    inline int end() const {
        return 0;
    }

    void print() {
        for (NodeSet::iterator it = begin(); it != end(); it++) {
            cout << (*it)->name << " ";
        }
    }
};

clock_t cutoff;
uint8_t cutoff2;
uint8_t cutoff3;
unordered_map<string, Node*> names;
NodeSet dom;
NodeSet dominated;
int g_seed = 76858720;

inline int fastRand() {
  g_seed = (214013 * g_seed + 2531011);
  return (g_seed >> 16) & 0x7FFF;
}

bool isDominated1(Node *node0) {
    for (int it = node0->nedges - 1; it >= 0; it--) {
        Node* node1 = node0->edges[it];
        if (dom.count(node1)) {
            return true;
        }
    }
    return false;
}

bool isDominated2(Node *node1, Node* node0) {
    if (dom.count(node1)) {
        return true;
    }
    for (int it = node1->nedges - 1; it >= 0; it--) {
        Node* node2 = node1->edges[it];
        if (node2 != node0 && dom.count(node2)) {
            return true;
        }
    }
    return false;
}

void refreshScore(Node *node0) {
    bool isDom = isDominated1(node0);
    int sum = isDom ? 0 : node0->freq;
    for (int it = node0->nedges - 1; it >= 0; it--) {
        Node* node1 = node0->edges[it];
        if (!isDominated2(node1, node0)) {
            sum += node1->freq;
        }
    }
    if (dom.count(node0)) {
        node0->score = -sum / node0->weight;
        dominated.insert(node0);
    } else {
        node0->score = sum / node0->weight;
        if (isDom) {
            dominated.insert(node0);
        } else {
            dominated.erase(node0);
        }
    }
}


void addNode(Node *node) {
    //cout << "adding " << node->name << " " << node->score << endl;
    dom.insert(node);
    refreshScore(node);
    for (int it = node->nedges - 1; it >= 0; it--) {
        Node* node2 = node->edges[it];
        refreshScore(node2);
    }
    //cout << node->score << endl;
}

/*void removeNode(Node* node) {
    nodes.erase(node);
    NodeSet::iterator it;
    for (it = node->edges.begin(); it != node->edges.end(); it++) {
        Node* node2 = *it;
        node2->edges.erase(node);
    }
    node->edges.clear();
}*/

/*void getN2(Node* node, NodeSet& n2) {
    for (NodeSet::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        for (NodeSet::iterator it2 = node1->edges.begin(); it2 != node->edges.end(); it2++) {
            Node* node2 = *it2;
            if (!n2.count(node2)) {
                n2.insert(node2);
            }
        }
    }
}

void updateConf(Node *node, int n0, int n1, int n2) {
    for (NodeSet::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        node1->conf = n1;
        for (NodeSet::iterator it2 = node1->edges.begin(); it2 != node->edges.end(); it2++) {
            Node *node2 = *it2;
            node2->conf = n2;
        }
    }
    for (NodeSet::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        node1->conf = n1;
    }
    node->conf = n0;
}*/

void loadData() {
	int t;
	cin >> t;
    cutoff = clock() + CLOCKS_PER_SEC * (t > 100 ? 5 : 2) - CLOCKS_PER_SEC / 50;
    cutoff2 = 1;
    cutoff3 = t > 100 ? 14 : 50;
    nnodes = t;

    for (int i = 0; i < t; i++) {
        Node * node = new Node(scores[i]);
        node->idA = i >> 6;
        node->idU = uint64_t(1) << (i & 63);
        node->idNU = ~node->idU;
        cin >> node -> name >> node -> weight;
        node -> freq = node -> weight;
        //node->conf = 1;
        nodeArr[i] = node;
        names[node->name] = node;
    }

    int m;
    cin >> m;
    for (int j = 0; j < m; j++) {
        string name1, name2;
        cin >> name1 >> name2;
        Node *node1 = names[name1];
        Node *node2 = names[name2];
        #ifdef VERIFY
        if (node1 == node2) {
            cout << "EQ " << node1 << endl;
        }
        #endif
        node1->edges[node1->nedges++] = node2;
        node2->edges[node2->nedges++] = node1;
    }
}

void findDominatingSet() {
    /*bool change = true;
    while(change) {
        change = false;
        NodeSet nodes2 = nodes;
        for (NodeSet::iterator it = nodes2.begin(); it != nodes2.end(); it++) {
            Node* node = *it;
            int size = node->edges.size();
            if (size == 0) {
                removeNode(node);
                dom.insert(node);
                node->conf = -1;
                change = true;
            };

            int total = 0;
            for (NodeSet::iterator it2 = node->edges.begin(); it2 != node->edges.end(); it2++) {
                Node *node2 = *it2;
                if (node2->edges.size() == 1) {
                    total += node2->weight;
                }
            };
            if (total > node->weight) {
                NodeSet edges = node->edges;
                for (NodeSet::iterator it2 = edges.begin(); it2 != edges.end(); it2++) {
                    Node *node2 = *it2;
                    if (node2->edges.size() == 1) {
                        removeNode(node2);
                    }
                }
                removeNode(node);
                dom.insert(node);
                change =true;
            }
        }
    }*/
    //cout << "start" << endl;
    for (NodeSet::niterator it = dominated.nbegin(); it != dominated.end(); it++) {
        refreshScore(*it);
    }
    while(dominated.size() != nnodes) {
        int bestScore = INT_MIN;
        Node *best;
        int equals = 1;
        for (NodeSet::niterator it = dom.nbegin(); it != dom.end(); it++) {
            Node *node = *it;
            int sc = node->score;
            //cout << node->name << " " << sc << " " << dom.count(node) << endl;
            if (sc > bestScore) {
                bestScore = sc;
                best = node;
                equals = 1;
            } else if (sc == bestScore && fastRand() % (++equals) == 0) {
                best = node;
            }
        }
        //if (!bestScore) {
          //  break;
        //}
        addNode(best);

        for (NodeSet::niterator it = dominated.nbegin(); it != dominated.end(); it++) {
            Node *node = *it;
            node->freq += node->weight;
        }
    }
}

void printResults();

void findDominatingSet2() {
    NodeSet best;
    int bestScore2 = 0x3ffffff;
    //int n = 0;
    while (true) {
        NodeSet fixed;
        int bestScore = 0x3ffffff;
        Node* bestNode;
        int scores2[MAX_NODES];
        NodeSet dominated2;
        while (true) {
            //cout << "start " << fixed.size() << endl;
            for (int it = 0; it < nnodes; it++) {
                //n++;
                dom = fixed;
                if (it) {
                    memcpy(scores, scores2, nnodes * sizeof(scores[0]));
                    dominated = dominated2;
                } else {
                    for (int i = 0; i < nnodes; i++) {
                        refreshScore(nodeArr[i]);
                    }
                    memcpy(scores2, scores, nnodes * sizeof(scores[0]));
                    dominated2 = dominated;
                }

                Node* node = nodeArr[it];
                addNode(node);
                findDominatingSet();

                #ifdef VERIFY
                for (int it = 0; it < nnodes; it++) {
                    Node *node = nodeArr[it];
                    bool isDom = isDominated2(node, NULL);
                    if (dom.count(node) && !isDom) {
                        cout << "A " << it << " " << isDom << " " << dom.count(node) << " "
                            << dominated.count(node) << " " << fixed.count(node) << endl;
                    }
                    if (dominated.count(node) != isDom) {
                        cout << "C " << it << " " << isDom << " " << dom.count(node) << " "
                            << dominated.count(node) << " " << fixed.count(node) << endl;
                    }
                    if (fixed.count(node) && !dom.count(node)) {
                        cout << "D " << it << " " << isDom << " " << dom.count(node) << " "
                            << dominated.count(node) << " " << fixed.count(node) << endl;
                    }
                }
                #endif

                int total = 0;
                for (NodeSet::iterator it2 = dom.begin(); it2 != dom.end(); it2++) {
                    Node *node2 = *it2;
                    total += node2->weight;
                }
                if (total < bestScore) {
                    bestScore = total;
                    bestNode = node;
                    if (total < bestScore2) {
                        bestScore2 = total;
                        best = dom;
                    }
                }
                //cout << bestScore << endl;
                if (!--cutoff2) {
                    cutoff2 = cutoff3;
                    if (clock() >= cutoff) {
                        dom = best;
                        //cout << "ALL " << n << endl;
                        return;
                    }
                }
            }
            if (fixed.count(bestNode)) {
                break;
            }
            fixed.insert(bestNode);
            //cout << "S " << fixed.size() << endl;
        }
        //g_seed++;
        //printResults();
        //break;
    }
}

/*int totalWeight(NodeSet& dom) {
    int total = 0;
    for (NodeSet::iterator it = dom.begin(); it != dom.end(); it++) {
        total += (*it)->weight;
    }
    return total;
}

Node* pickRandom(NodeSet& s) {
    while (true) {
        int i = fastRand() % s.size();
        NodeSet::iterator it = begin(s);
        if (i) {
            advance(it, i);
        }
        Node* node = *it;
        if (node->conf >= 0) {
            return node;
        }
    }
}

Node* bms(int step) {
    int k = double(fastRand()) / 32767 < exp(-step) ? 1024 : 50 + (fastRand() % 10);
    Node* u2 = pickRandom(dom);
    double score1 = score(u2);
    Node* u = u2;
    for (int i = 0; i < k; i++) {
        u2 = pickRandom(dom);
        double score2 = score(u2);
        if (score2 > score1) {
            score1 = score2;
            u = u2;
        }
    }
    return u;
}

void removeFromDom(Node* node) {
    dom.erase(node);
    if (!isDominated(node, node)) {
        ndom.insert(node);
    }
    updateConf(node, 1, 2, 2);
    for (NodeSet::iterator it = node->edges.begin(); it != node->edges.end(); it++) {
        Node *node2 = *it;
        if (!isDominated(node2, node)) {
            ndom.insert(node);
        }
    }
}

void print() {
    for (NodeSet::iterator it = dom.begin(); it != dom.end(); it++) {
        Node* node = *it;
        cout << node->name << " " << node->conf << " " << score(node) << endl;
    }
}

void optimize() {
    NodeSet dom2 = dom;
    int nonImpr = 0;
    int totalW = totalWeight(dom);
    int totalW2 = totalW;
    while (clock() < cutoff) {
    //cout << "a " << ndom.size() << endl;
        NodeSet dom3 = dom;
        double maxScore = -1e30;
        Node* maxScoreNode = NULL;
        for (NodeSet::iterator it = dom3.begin(); it != dom3.end(); it++) {
            Node *node = *it;
            if (node->conf < 0) {
                continue;
            }
            double sc = score(node);
            if (sc == 0) {
                dom.erase(node);
                if (!isDominated(node))
                    ndom.insert(node);
                totalW -= node->weight;
                break;
            }
            else {
                node->conf = 0;
                if (sc > maxScore) {
                    maxScore = sc;
                    maxScoreNode = node;
                }
            }
        }

        if (totalW < totalW2) {
   //cout << "T " <<totalW << " " <<totalW2 << endl;
            dom2 = dom;
            totalW2 = totalW;
            nonImpr = 0;
        }

    //print();
    //cout << "b1 " << ndom.size() << " " <<maxScoreNode << endl;
        //if (maxScoreNode) {
        //    removeFromDom(maxScoreNode);
        //    totalW -= maxScoreNode->weight;
        //}
    //cout << "b2 " << ndom.size() << endl;

        Node* bmsNode = bms(nonImpr);
    //cout << "b3 " << ndom.size() << endl;
    //cout << "f " << maxScoreNode->name << " "  << bmsNode->name << " " << nonImpr << endl;
        removeFromDom(bmsNode);
        totalW -= bmsNode->weight;

        do {
    //cout << "b " << ndom.size() << " " << maxScoreNode << " " << bmsNode << endl;
            //print();
            NodeSet n2;
            //getN2(maxScoreNode, n2);
            getN2(bmsNode, n2);
    //cout << "c " << endl;
            double score1 = -1e30;
            Node* best = NULL;
            for (NodeSet::iterator it = n2.begin(); it != n2.end(); it++) {
    //cout << "c1 " << endl;
                Node* node = *it;
                if (node->conf <= 0) {
                    continue;
                }
                double score2 = score(node);
    //cout << "c2 " << node << " " << best << endl;
                if (score2 != 0 && (score2 > score1 || (score2 == score1 && node->conf > best->conf))) {
                    score1 = score2;
                    best = node;
                }
            }
    //cout << "c3 " << endl;
            if (!best)
                break;
            //cout << best <<  " " << ndom.size() << endl;
    //cout << "d " << best->name << " " << best->conf << endl;

            dom.insert(best);
    //cout << __LINE__ <<endl;
            ndom.erase(best);
            totalW += best->weight;
    //cout << __LINE__ <<endl;
            for (NodeSet::iterator it2 = best->edges.begin(); it2 != best->edges.end(); it2++) {
    //cout << __LINE__ <<endl;
                ndom.erase(*it2);
    //cout << __LINE__ <<endl;
            };
    //cout << "e " << ndom.size() << endl;

            updateConf(best, 0, 1, 2);
        } while(ndom.size() > 0);
    //cout << "b4 " << ndom.size() << endl;
        nonImpr++;
    }
}*/

void printResults() {
    cout << dom.size() << endl;
    int total = 0;
    for (NodeSet::iterator it = dom.begin(); it != dom.end(); it++) {
        Node *node = *it;
        cout << node->name << endl;
        total += node->weight;
    }
    cout << total << endl;
}

void freeMemory() {
    for (int it = 0; it < nnodes; it++) {
        delete nodeArr[it];
    }
    dom.clear();
    names.clear();
}

/*void createTestCase(int test) {
    const int NCASES = 10;
    int nodes[NCASES] = {100, 100, 100, 100, 100, 300, 300, 300, 300, 300};
    int conn[NCASES] = {99, 101, 105, 114, 130, 299, 302, 311, 339, 404};
    cout << nodes[test] << endl;
    for (int i = 0; i < nodes[test]; i++) {
        cout << i << " " << (rand() % 249 + 1) << endl;
    }
    cout << conn[test] << endl;
    for (int i = 0; i < conn[test]; i++) {
        int a = rand() % nodes[test], b;
        do {
            b = rand() % nodes[test];
        } while(a == b);
        cout << a << " " << b << endl;
    }
}

int main(int argc, char **argv) {
    createTestCase(atoi(argv[1]));
}*/

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 3);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main() {
    //signal(SIGSEGV, handler);
    //signal(SIGBUS, handler);
    loadData();
    /*if (nodes.size() <= 20) {
        bruteForce();
    } else {*/
        findDominatingSet2();
        //optimize();
    //}
    printResults();
    //freeMemory();
    return 0;
}
