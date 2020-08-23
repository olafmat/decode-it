#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <stdlib.h>
#include <math.h>
using namespace std;

struct Node {
    string name;
    int weight;
    unordered_set<Node*> edges;
    int freq;
    int conf;
};

clock_t cutoff;
unordered_set<Node*> all;
unordered_set<Node*> nodes;
unordered_map<string, Node*> names;
set<Node*> dom;
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

bool isDominated(Node *node, Node* excl = NULL) {
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

void getN2(Node* node, unordered_set<Node*>& n2) {
    for (unordered_set<Node*>::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        for (unordered_set<Node*>::iterator it2 = node1->edges.begin(); it2 != node->edges.end(); it2++) {
            Node* node2 = *it2;
            if (!n2.count(node2)) {
                n2.insert(node2);
            }
        }
    }
}

void updateConf(Node *node, int n0, int n1, int n2) {
    for (unordered_set<Node*>::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        node1->conf = n1;
        for (unordered_set<Node*>::iterator it2 = node1->edges.begin(); it2 != node->edges.end(); it2++) {
            Node *node2 = *it2;
            node2->conf = n2;
        }
    }
    for (unordered_set<Node*>::iterator it1 = node->edges.begin(); it1 != node->edges.end(); it1++) {
        Node* node1 = *it1;
        node1->conf = n1;
    }
    node->conf = n0;
}

double score(Node *node) {
    int sum = isDominated(node, node) ? 0 : node->freq;
    unordered_set<Node*>::iterator it;
    for (it = node->edges.begin(); it != node->edges.end(); it++) {
        Node* node2 = *it;
        if (!isDominated(node2, node)) {
            sum += node2->freq;
        }
    }
    return dom.count(node) ? -double(sum) / node->weight : double(sum) / node->weight;
}


void loadData() {
	int t;
	cin >> t;
    cutoff = clock() + CLOCKS_PER_SEC * (t > 100 ? 5 : 3) - CLOCKS_PER_SEC / 20;

    for (int i = 0; i < t; i++) {
        Node * node = new Node();
        cin >> node -> name >> node -> weight;
        node->freq = node->conf = 1;
        all.insert(node);
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
    set<Node*> bestDom;
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
    /*bool change = true;
    while(change) {
        change = false;
        unordered_set<Node*> nodes2 = nodes;
        for (unordered_set<Node*>::iterator it = nodes2.begin(); it != nodes2.end(); it++) {
            Node* node = *it;
            int size = node->edges.size();
            if (size == 0) {
                removeNode(node);
                dom.insert(node);
                node->conf = -1;
                ndom.erase(node);
                change = true;
            };

            int total = 0;
            for (unordered_set<Node*>::iterator it2 = node->edges.begin(); it2 != node->edges.end(); it2++) {
                Node *node2 = *it2;
                if (node2->edges.size() == 1) {
                    total += node2->weight;
                }
            };
            if (total > node->weight) {
                unordered_set<Node*> edges = node->edges;
                for (unordered_set<Node*>::iterator it2 = edges.begin(); it2 != edges.end(); it2++) {
                    Node *node2 = *it2;
                    if (node2->edges.size() == 1) {
                        removeNode(node2);
                    }
                }
                removeNode(node);
                dom.insert(node);
                ndom.erase(node);
                change =true;
            }
        }
    }*/

    bool repeat = !ndom.empty();
    while(repeat) {
        double bestScore = -1e30;
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
        updateConf(best, 0, 1, 2);
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
}

void findDominatingSet2() {
    set<Node*> best;
    int bestScore = 0x3ffffff;
    for (unordered_set<Node*>::iterator it = all.begin(); it != all.end(); it++) {
        dom.clear();
        ndom = nodes;
        dom.insert(*it);
        ndom.erase(*it);
        findDominatingSet();
        int total = 0;
        for (set<Node*>::iterator it = dom.begin(); it != dom.end(); it++) {
            Node *node = *it;
            total += node->weight;
        }
        if (total < bestScore) {
            bestScore = total;
            best = dom;
        }
    }
    dom = best;
}

int totalWeight(set<Node*>& dom) {
    int total = 0;
    for (set<Node*>::iterator it = dom.begin(); it != dom.end(); it++) {
        total += (*it)->weight;
    }
    return total;
}

Node* pickRandom(set<Node*>& s) {
    while (true) {
        int i = random() % s.size();
        set<Node*>::iterator it = begin(s);
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
    int k = double(random()) / RAND_MAX < exp(-step) ? 1024 : 50 + (random() % 10);
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
    for (unordered_set<Node*>::iterator it = node->edges.begin(); it != node->edges.end(); it++) {
        Node *node2 = *it;
        if (!isDominated(node2, node)) {
            ndom.insert(node);
        }
    }
}

void print() {
    for (set<Node*>::iterator it = dom.begin(); it != dom.end(); it++) {
        Node* node = *it;
        cout << node->name << " " << node->conf << " " << score(node) << endl;
    }
}

void optimize() {
    set<Node*> dom2 = dom;
    int nonImpr = 0;
    int totalW = totalWeight(dom);
    int totalW2 = totalW;
    while (clock() < cutoff) {
    //cout << "a " << ndom.size() << endl;
        set<Node*> dom3 = dom;
        double maxScore = -1e30;
        Node* maxScoreNode = NULL;
        for (set<Node*>::iterator it = dom3.begin(); it != dom3.end(); it++) {
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
        /*if (maxScoreNode) {
            removeFromDom(maxScoreNode);
            totalW -= maxScoreNode->weight;
        }*/
    //cout << "b2 " << ndom.size() << endl;

        Node* bmsNode = bms(nonImpr);
    //cout << "b3 " << ndom.size() << endl;
    //cout << "f " << maxScoreNode->name << " "  << bmsNode->name << " " << nonImpr << endl;
        removeFromDom(bmsNode);
        totalW -= bmsNode->weight;

        do {
    //cout << "b " << ndom.size() << " " << maxScoreNode << " " << bmsNode << endl;
            //print();
            unordered_set<Node*> n2;
            //getN2(maxScoreNode, n2);
            getN2(bmsNode, n2);
    //cout << "c " << endl;
            double score1 = -1e30;
            Node* best = NULL;
            for (unordered_set<Node*>::iterator it = n2.begin(); it != n2.end(); it++) {
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
            for (unordered_set<Node*>::iterator it2 = best->edges.begin(); it2 != best->edges.end(); it2++) {
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
}

void printResults() {
    cout << dom.size() << endl;
    int total = 0;
    for (set<Node*>::iterator it = dom.begin(); it != dom.end(); it++) {
        Node *node = *it;
        cout << node->name << endl;
        total += node->weight;
    }
    cout << total << endl;
}

void freeMemory() {
    for (unordered_set<Node*>::iterator it = all.begin(); it != all.end(); it++) {
        Node *node = *it;
        node->edges.clear();
        delete node;
    }
    all.clear();
    nodes.clear();
    dom.clear();
    ndom.clear();
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

int main() {
    loadData();
    /*if (nodes.size() <= 20) {
        bruteForce();
    } else {*/
        findDominatingSet2();
        //optimize();
    //}
    printResults();
    freeMemory();
    return 0;
}

//0 4855
//1 4757
//2 4605
//3 4186
//4 4540
//5 17327
//6 16982
//7 16953
//8 15811
//9 14118

