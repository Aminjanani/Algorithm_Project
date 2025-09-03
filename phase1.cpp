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
    int to, rev;
    int cap, cost;
    Edge(int _to, int _rev, int _cap, int _cost) : to(_to), rev(_rev), cap(_cap), cost(_cost) {}
};

class mcmf {
private:
    vector<vector<Edge>> g;
    vector<taskNode> tasks;
    vector<Node> nodes;
    map<pair<string, string>, int> costs;
    map<string, string> assignedTasks;
    int numOfTasks, numOfNodes;
    int N;

public:
    mcmf(int n, int t, vector<Node>& NODES, vector<taskNode>& TASKS, map<pair<string, string>, int>& C) 
        : tasks(TASKS), nodes(NODES), costs(C) {
        numOfTasks = tasks.size();
        numOfNodes = nodes.size();
        N = numOfTasks + numOfNodes + 2;
        g.assign(N, {});
        for (int i = 1; i <= numOfTasks; i++) addEdge(0, i, 1, 0);
        for (int i = 1; i <= numOfTasks; i++) {
            for (int j = 1; j <= numOfNodes; j++) {
                if (tasks[i - 1].cpu <= nodes[j - 1].cpu_cap && tasks[i - 1].ram <= nodes[j - 1].ram_cap) {
                    int cost = costs[{tasks[i - 1].name, nodes[j - 1].name}];
                    addEdge(i, numOfTasks + j, 1, cost);
                }
            }
        }
        for (int j = 1; j <= numOfNodes; j++) addEdge(numOfTasks + j, N - 1, INT_MAX, 0);
    }

    pair<int, int> minCostMaxFlow() {
        int flow = 0, cost = 0;
        vector<int> h(N, 0), dist(N), prevv(N), preve(N);
        while (true) {
            fill(dist.begin(), dist.end(), INF);
            dist[0] = 0;
            priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
            pq.push({0, 0});
            while (!pq.empty()) {
                auto [d, v] = pq.top(); 
                pq.pop();
                if (dist[v] < d) continue;
                for (int i = 0; i < g[v].size(); i++) {
                    Edge &e = g[v][i];
                    if (e.cap > 0 && dist[e.to] > dist[v] + e.cost + h[v] - h[e.to]) {
                        dist[e.to] = dist[v] + e.cost + h[v] - h[e.to];
                        prevv[e.to] = v;
                        preve[e.to] = i;
                        pq.push({dist[e.to], e.to});
                    }
                }
            }
            if (dist[N - 1] == INF) break;
            for (int v = 0; v < N; v++) if (dist[v] < INF) h[v] += dist[v];
            int d = INT_MAX;
            for (int v = N - 1; v != 0; v = prevv[v]) d = min(d, g[prevv[v]][preve[v]].cap);
            flow += d;
            cost += d * h[N - 1];
            for (int v = N - 1; v != 0; v = prevv[v]) {
                Edge &e = g[prevv[v]][preve[v]];
                e.cap -= d;
                g[v][e.rev].cap += d;
            }
        }
        for (int i = 1; i <= numOfTasks; i++) {
            for (auto &e : g[i]) {
                if (e.to > numOfTasks && e.to < N - 1 && e.cap == 0) {
                    assignedTasks[tasks[i - 1].name] = nodes[e.to - numOfTasks - 1].name;
                }
            }
        }
        return {cost, flow};
    }

    map<string, string> getAssginedTasks() {
        return assignedTasks;
    }

private:
    void addEdge(int from, int to, int cap, int cost) {
        g[from].push_back(Edge(to, g[to].size(), cap, cost));
        g[to].push_back(Edge(from, g[from].size() - 1, 0, -cost));
    }
};

int main() {
    string file_name = "input1.json";
    ifstream file(file_name);
    if (!file.is_open()) {
        cout << "Error: Could not open input file!" << '\n';
        return 1;
    }
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
    if (ofile.is_open()) {
        ofile << output.dump(2) << '\n';
        ofile.close();
    } else {
        cout << "Error: Could not open output file!" << '\n';
    }

    return 0;
}
