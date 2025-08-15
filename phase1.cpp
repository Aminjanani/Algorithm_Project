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
    //vector<string> adj;

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
    pair<int, int> cap;
    vector<int> adj;

    Node(int ID, string NAME, int CPU_CAP, int RAM_CAP) {
        id = ID;
        name = NAME;
        cpu_cap = CPU_CAP;
        ram_cap = RAM_CAP;
        cap = {CPU_CAP, RAM_CAP};
        adj = {};
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
    map<pair<string, string>, int> costs;
    map<string, string> assignedTasks;
    int numOfTasks, numOfNodes;

public:
    mcmf(int n, int t, vector<Node>& N, vector<taskNode>& T,
         map<pair<string, string>, int>& C) : tasks(T), nodes(N), costs(C) {
        numOfTasks = tasks.size();
        numOfNodes = nodes.size();

        int totalSize = numOfTasks + numOfNodes + 2;
        g.resize(totalSize, vector<Edge*>(totalSize, nullptr));

        // Source edges to tasks
        for (int i = 1; i <= numOfTasks; i++) {
            g[0][i] = new Edge(0, i, 0, 1);
        }

        // Edges from tasks to nodes
        for (int i = 1; i <= numOfTasks; i++) {
            for (int j = numOfTasks + 1; j <= numOfTasks + numOfNodes; j++) {
                addEdge(i, j - numOfTasks, costs[{tasks[i - 1].name, nodes[j - numOfTasks - 1].name}]);
                if (tasks[i - 1].cpu <= nodes[j - numOfTasks - 1].cpu_cap && tasks[i - 1].ram <= nodes[j - numOfTasks - 1].ram_cap) {
                    paths.insert({{i, j}, costs[{tasks[i - 1].name, nodes[j - numOfTasks - 1].name}]});
                    nodes[j - numOfTasks - 1].adj.push_back(i - 1);
                }
            }
        }

        // Node edges to sink
        for (int i = numOfTasks + 1; i <= numOfTasks + numOfNodes; i++) {
            g[i][numOfTasks + numOfNodes + 1] = new Edge(i, numOfTasks + numOfNodes + 1, 0, INT_MAX);
        }
    }

    ~mcmf() {
        for (auto& row : g) {
            for (auto& edge : row) {
                delete edge;
                edge = nullptr;
            }
        }
    }

    void addEdge(int tId, int nId, int cost) {
        if (tasks[tId - 1].cpu <= nodes[nId - 1].cpu_cap && tasks[tId - 1].ram <= nodes[nId - 1].ram_cap) {
            int from = tId;
            int to = nId + numOfTasks;
            g[from][to] = new Edge(from, to, cost, 1);
        }
    }

    void removeUseLessEdges(int taskID) {
        for (int i = numOfTasks + 1; i <= numOfTasks + numOfNodes; i++) {
            if (g[taskID][i] != nullptr) {
                delete g[taskID][i];
                g[taskID][i] = nullptr;
                pair<int, int> target = {taskID, i};
                for (auto it = paths.begin(); it != paths.end();) {
                    if (it->first == target) {
                        it = paths.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
    }

    pair<pair<int, int>, int> findShortestPath() {
        auto it = paths.begin();
        if (it == paths.end()) {
            return {{-1, -1}, -1};
        }

        pair<pair<int, int>, int> min_element = *it;
        int taskId = min_element.first.first;

        removeUseLessEdges(taskId);

        int i = min_element.first.first;
        int j = min_element.first.second;
        int new_cpu = nodes[j - numOfTasks - 1].cap.first - tasks[i - 1].cpu;
        int new_ram = nodes[j - numOfTasks - 1].cap.second - tasks[i - 1].ram;
        nodes[j - numOfTasks - 1].cap = {new_cpu, new_ram};
        for (int k = 0; k < nodes[j - numOfTasks - 1].adj.size(); k++) {
            int cpu = tasks[k].cpu;
            int ram = tasks[k].ram;
            if (cpu > new_cpu || ram > new_ram) {
                pair<int, int> target = {k, j};
                for (auto it = paths.begin(); it != paths.end();) {
                    if (it->first == target) {
                        it = paths.erase(it);
                    } else {
                        ++it;
                    }
                } 
            }
        }

        return min_element;
    }

    void displayPaths() {
        while (paths.size()) {
            auto it = paths.begin();
            pair<pair<int, int>, int> min_element = *it;
            cout << "(" << min_element.first.first << ", " << min_element.first.second << ")" << ", cost : " << min_element.second << '\n';
            paths.erase(it);
        }
    }

    pair<int, int> minCostMaxFlow() {
        int min_cost = 0, max_flow = 0;
        while (!paths.empty()) {
            pair<pair<int, int>, int> res = findShortestPath();
            int taskId = res.first.first;
            int nodeId = res.first.second;
            
            if (taskId == -1) {
                break;
            }

            min_cost += res.second;
            max_flow++;

            if (g[0][taskId] != nullptr) {
                delete g[0][taskId];
                g[0][taskId] = nullptr;
            }

            //removeUseLessEdges(taskId);
            assignedTasks[tasks[taskId - 1].name] = nodes[nodeId - numOfTasks - 1].name;
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
        taskNode tn(num++, name, cpu, ram, deadline);
        taskNodes.push_back(tn);
        //num++;
    }

    vector<Node> Nodes;
    json nodes = input["nodes"];
    for (int i = 0; i < nodes.size(); i++) {
        string name = nodes[i]["id"];
        N.push_back(name);
        int cpu_cap = nodes[i]["cpu_capacity"];
        int ram_cap = nodes[i]["ram_capacity"];
        Node node(num++, name, cpu_cap, ram_cap);
        Nodes.push_back(node);
        //num++;
    }

    json exec_cost = input["exec_cost"];
    map<pair<string, string>, int> exec_cost_mp;
    for (auto& task_name : T) {
        for (auto& node_name : N) {
            exec_cost_mp[{task_name, node_name}] = INF;
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
    mcmf MF(n, t, Nodes, taskNodes, exec_cost_mp);
    auto result = MF.minCostMaxFlow();

    cout << "min cost: " << result.first << ", max flow: " << result.second << '\n';

    auto assignments = MF.getAssginedTasks();
    for (const auto& pair : assignments) {
        cout << "Task " << pair.first << " assigned to Node " << pair.second << '\n';
    }

    json output;
    output["assignments"] = json::object();
    for (auto& p : assignments) {
        output["assignments"][p.first] = p.second;
    }

    output["total_cost"] = result.first;

    string outputFileName = "phase1_output.json";
    ofstream ofile(outputFileName);
    ofile << output.dump(2);
    ofile.close();

    return 0;
}
