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
    int duration;

    taskNode(int ID, string NAME, int CPU, int RAM, int DEADLINE, int DURATION) {
        id = ID;
        name = NAME;
        cpu = CPU;
        ram = RAM;
        deadline = DEADLINE;
        duration = DURATION;
    }
};

struct Node {
    int id;
    string name;
    int cpu_cap;
    int ram_cap;

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

class timeExpandedMCMF {

};

int main() {
    string file_name = "input2.json";
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
        int duration = tasks[i]["duration"];
        taskNode tn(num++, name, cpu, ram, deadline, duration);
        taskNodes.push_back(tn);
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

    vector<int> timeStamps = input["time_slots"];
    for (int i = 0; i < timeStamps.size(); i++) {
        //cout << timeStamps[i] + 1 << ' ';
    }
    //cout << '\n';

    map<pair<string, int>, int> capacity_per_time_slot;
    for (auto& [node, timeslot_capacity] : input["node_capacity_per_time"].items()) {
        for (auto& [time_slot, cap] : timeslot_capacity.items()) {
            int ts = stoi(time_slot);
            capacity_per_time_slot[{node, ts}] = cap;
            //cout << "Node: " << node <<  ", Time Slot: " << ts << ", Capacity: " << cap << '\n';
        }
    }

    map<string, int> task_mp;
    for (int i = 0; i < T.size(); i++) {
        task_mp[T[i]] = i;
        cout << "Task: " << T[i] << ", id: " << task_mp[T[i]] << '\n';
    }

    vector<vector<int>> dependency_graph(T.size() + 1);
    vector<int> deg(T.size() + 1, 0);
    //map<string, string> dependencies;
    for (int i = 0; i < input["dependencies"].size(); i++) {
        string bef = input["dependencies"][i]["before"];
        string aft = input["dependencies"][i]["after"];
        int bef_id = task_mp[bef];
        int aft_id = task_mp[aft];
        deg[aft_id + 1]++;
        dependency_graph[bef_id + 1].push_back(aft_id + 1);
        //dependencies[bef] = aft;

        //cout << "You should first do the task " << bef << " before starting the task " << aft << '\n';
    }

    for (int i = 1; i < T.size(); i++) {
        if (deg[i] == 0) {
            dependency_graph[0].push_back(i);
        }
    }

    for (int i = 0; i < T.size(); i++) {
        cout << i << ": {";
        for (auto x : dependency_graph[i]) {
            cout << x << " ";
        }
        cout << "}\n";
    }

    vector<vector<int>> pq(T.size());
    vector<bool> vis(T.size() + 1, false);
    vector<int> dist(T.size() + 1);
    int idx = 1;
    auto bfs = [&](int root) {
        vis[root] = true;
        dist[root] = 0;
        queue<int> q;
        q.push(root);
        while (q.size()) {
            int v = q.front();
            q.pop();
            for (auto x : dependency_graph[v]) {
                if (!vis[x]) {
                    vis[x] = true;
                    dist[x] = dist[v] + 1;
                    if (idx < dist[x]) {
                        idx = dist[x];
                    }
                    pq[dist[x] - 1].push_back(x);
                    q.push(x);
                }
            }
        }
    };

    int ext = T.size() - idx;
    while (--ext) {
        pq.pop_back();
    }

    bfs(0);
    for (int i = 0; i < pq.size(); i++) {
        for (auto x : pq[i]) {
            cout << x << " ";
        }
        cout << '\n';
    }

    cout << idx << '\n';

    return 0;
}
