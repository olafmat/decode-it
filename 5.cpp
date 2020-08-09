#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
using namespace std;

struct Node {
    string name;
    int weight;
    vector<Node*> edges;
};

vector<Node*> nodes;
unordered_map<string, Node*> names;

void loadData() {
	int t;
	cin >> t;

    for (int i = 0; i < t; i++) {
        Node * node = new Node();
        cin >> node -> name >> node -> weight;
        nodes.push_back(node);
        names[node->name] = node;
    }

    int m;
    cin >> m;
    for (int j = 0; j < m; j++) {
        string name1, name2;
        cin >> name1 >> name2;
        Node *node1 = names[name1];
        Node *node2 = names[name1];
        node1->edges.push_back(node2);
        node2->edges.push_back(node1);
    }
}

int main() {
    loadData();
	return 0;
}
