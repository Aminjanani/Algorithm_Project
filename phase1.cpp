#include <bits/stdc++.h>
#include <optional>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

const int INF = 1e9 + 7;

struct taskNode {
    int id;
    string name;
    int cpu;
    int ram;
    int deadline;
    vector<string> adj;

    taskNode(int ID, string NAME, int CPU, int RAM, int DEADLINE) {
        id = ID;
        name = NAME;
        cpu = CPU; 
        ram = RAM;  
        deadline = DEADLINE; 
    }
};

struct Node {
    int id;
    string name;
    int cpu_cap;
    int ram_cap;
    vector<string> adj;

    Node(int ID, string NAME, int CPU_CAP, int RAM_CAP) {
        id = ID;
        name = NAME;
        cpu_cap = CPU_CAP; 
        ram_cap = RAM_CAP; 
    }
};

struct Edge {
    int taskId;
    int nodeId;
    int cost;
    int capacity;
    bool edgeExists = false;

    Edge(int TaskID, int NodeID, int COST, int CAP) {
        cost = COST; 
        capacity = CAP;
        taskId = TaskID;
        nodeId = NodeID;
        edgeExists = true;
    }
};

struct comp {
    bool operator()(const pair<pair<int, int>, int>& x, const pair<pair<int, int>, int>& y) {
        return x.second < y.second;
    }
};

// min_cost-max_flow class
class mcmf {
    private:
        set<pair<pair<int, int>, int>, comp> paths;
        vector<vector<Edge*>> g;
        vector<taskNode> tasks;
        vector<Node> nodes;
        vector<int> nodeCaps;
        map<pair<string, string>, int> costs;
        map<string, string> assignedTasks;
        int numOfTasks, numOfNodes;

    public:   
        mcmf(int n, int t, vector<Node>& N, vector<taskNode>& T,
             map<pair<string, string>, int>& C, vector<int>& NC) : tasks(T), nodes(N), costs(C), nodeCaps(NC) {
            numOfTasks = tasks.size(), numOfNodes = nodes.size();
            g.resize(n + 1, vector<Edge*>(n + t + 2, nullptr));
            for (int i = 1; i <= n; i++) {
                g[0][i] = new Edge(0, i, 0, 1);
            }
            for (int i = 1; i <= n; i++) {
                for (int j = n + 1; j <= n + t; j++) {
                    addEdge(i - 1, j - n - 1, costs[{tasks[i - 1].name, nodes[j - n - 1].name}]);
                    paths.insert({{i - 1, j - n - 1}, costs[{tasks[i - 1].name, nodes[j - n - 1].name}]});
                }
            }
            for (int i = n + 1; i < n + t + 1; i++) {
                g[i][n + t + 1] = new Edge(i, n + t + 1, 0, nodeCaps[i - t - 1]);
            }
        }
    
        void addEdge(int tId, int nId, int cost) {
            if (tasks[tId].cpu <= nodes[nId].cpu_cap && tasks[tId].ram <= nodes[nId].ram_cap) {
                g[tId + 1][nId + tasks.size() + 1] = new Edge(tId, nId, cost, 1);
            }
        }
        void removeUseLessEdges(int taskID) {
            for (int i = numOfTasks + 1; i < numOfTasks + numOfNodes + 1; i++) {
                if (g[taskID][i] != nullptr) {
                    delete g[taskID][i];
                    g[taskID][i] = nullptr;
                    pair<int, int> target = {taskID, i};
                    auto lB = paths.lower_bound({target, INT_MIN});
                    auto upB = paths.upper_bound({target, INT_MAX});
                    paths.erase(lB, upB);
                }
            }
        }
        pair<pair<int, int>, int> findShortestPath() {
            auto it = paths.begin();
            pair<pair<int, int>, int> min_element = *it;
            paths.erase(it);
            int taskId = min_element.first.first, nodeId = min_element.first.second, cost = min_element.second;
            removeUseLessEdges(taskId);
            return {{taskId, nodeId}, cost};
        }
        pair<int, int> minCostMaxFlow() {
            int min_cost = 0, max_flow = 0;
            int n = nodes.size(), t = tasks.size();
            while(!paths.size()) {
                pair<pair<int, int>, int> res = findShortestPath();
                int taskId = res.first.first, nodeId = res.first.second;
                min_cost += res.second;
                max_flow++;
                //removeUseLessEdges(0);
                delete g[0][taskId];
                g[0][taskId] = nullptr;
                removeUseLessEdges(taskId);
            }

            return {min_cost, max_flow};
        }
        map<string, string> getAssginedTasks() {
            return assignedTasks;
        }
};

int main() {
    string file_name = "input1.json";
    ifstream file(file_name);
    json input;
    file >> input;

    vector<string> T, N;

    int num = 1;
    json tasks = input["tasks"];
    vector<taskNode> taskNodes;
    for (int i = 0; i < tasks.size(); i++) {
        string name = tasks[i]["id"];
        T.push_back(name);
        int cpu = tasks[i]["cpu"];
        int ram = tasks[i]["ram"];
        int deadline = tasks[i]["deadline"];
        taskNode tn(num, name, cpu, ram, deadline);
        taskNodes.push_back(tn);
        num++;
    }
    
    vector<Node> Nodes;
    json nodes = input["nodes"];
    for (int i = 0; i < nodes.size(); i++) {
        string name = nodes[i]["id"];
        N.push_back(name);
        int cpu_cap = nodes[i]["cpu_capacity"];
        int ram_cap = nodes[i]["ram_capacity"];
        Node node(num, name, cpu_cap, ram_cap);
        Nodes.push_back(node);
        num++;
    }

    json exec_cost = input["exec_cost"];
    map<pair<string, string>, int> exec_cost_mp;
    for (auto s0 : T) {
        for (auto s1 : N) {
            exec_cost_mp[{s0, s1}] = -1;
        }
    }

    for (auto i = exec_cost.begin(); i != exec_cost.end(); ++i) {
        string s = i.key();
        for (auto j = exec_cost[s].begin(); j != exec_cost[s].end(); ++j) {
            pair<string, string> p = make_pair(s, j.key());
            exec_cost_mp[p] = j.value();
        }
    }

    int n = Nodes.size(), t = taskNodes.size();
    vector<int> nodeCaps(n, 0);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < t; j++) {
            if (Nodes[i].cpu_cap >= taskNodes[j].cpu && Nodes[i].ram_cap >= taskNodes[j].ram) {
                nodeCaps[i]++;
            }
        }
    }

    mcmf MF(n, t, Nodes, taskNodes, exec_cost_mp, nodeCaps);
    cout << MF.minCostMaxFlow().first << " " << MF.minCostMaxFlow().second << '\n';
    
    return 0;
}