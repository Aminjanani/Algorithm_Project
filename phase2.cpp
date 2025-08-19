#include <bits/stdc++.h>
#include <optional>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

const int INF = 1e9 + 7;
const int MINF = -INF;

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

struct comp {
    bool operator()(const pair<pair<int, int>, pair<int, int>>& x, const pair<pair<int, int>, pair<int, int>>& y) {
        if (x.second.first == y.second.first) {
            return x.second.second < y.second.second;
        }
        return x.second.second < y.second.second;
    }
};

class timeExpandedMCMF {
private:
    set<pair<pair<int, int>, pair<int, int>>, comp> paths;
    vector<int> leastStart, mostStart;
    vector<taskNode> tasks;
    vector<Node> nodes;
    map<pair<string, string>, int> costs;
    int numberOfTasks, numberOfNodes, nTS;
    vector<vector<int>> pq;
    vector<int> ts;
    map<pair<string, int>, int> timeSlotCap;
    vector<vector<int>> dependency_graph;
    map<string, pair<string, int>> assignedTasks;

public:
    timeExpandedMCMF(vector<int> TS, 
                     vector<vector<int>> PQ, 
                     map<pair<string, int>, int> capacity_per_time_slot,
                     vector<vector<int>> dg): 
                     ts(TS), pq(PQ), timeSlotCap(capacity_per_time_slot), dependency_graph(dg) {
        numberOfTasks = tasks.size();
        numberOfNodes = nodes.size();
        leastStart.resize(tasks.size());
        mostStart.resize(tasks.size());
        nTS = ts.size();
        int pqSize = pq.size();
        vector<int> firstPriority = pq[pqSize - 1];
        for (auto &x : firstPriority) {
            leastStart[x - 1] = 0;
            mostStart[x - 1] = tasks[x - 1].deadline - tasks[x - 1].duration;
        }
        for (int i = 0; i < pqSize - 1; i++) {
            for (auto &y : pq[i]) {
                leastStart[y - 1] = MINF;
                mostStart[y - 1] = tasks[y - 1].deadline - tasks[y - 1].duration;
            }
        }
    }
private:
    void updateNodeCapPerTime(int nodeId, int timeSlot, int cpu) {
        timeSlotCap[{nodes[nodeId].name, timeSlot}] -= cpu;
    }
    
    bool createPath(vector<int>& currentPriority) {
        pq.pop_back();
        for (auto x : currentPriority) {
            x--;
            for (int i = leastStart[x]; i <= mostStart[x]; i++) {
                for (int j = 0; j < numberOfNodes; j++) {
                    if (tasks[x].cpu <= timeSlotCap[{nodes[j].name, i}]) {
                        paths.insert({{x * nTS + i + 1, j + (numberOfTasks * nTS) + 1}, 
                                     {costs[{tasks[x].name, nodes[j].name}], i}});
                        updateNodeCapPerTime(j, i, tasks[x].cpu);             
                    }
                }
            }
        }

        if (!paths.empty()) {
            return true;
        } else {
            return false;
        }
    }

    void setStartTime(int taskId, int time) {
        for (auto &x : dependency_graph[taskId]) {
            leastStart[x] = time;
        }
    }

    void removeUselessPaths(int Id) {
        int taskId = ((Id - 1) / nTS);
        for (int i = taskId * nTS + 1; i <= (taskId + 1) * nTS; i++) {
            for (auto it = paths.begin(); it != paths.end();) {
                if (it->first.first == i) {
                    it = paths.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    pair<pair<int, int>, pair<int, int>> findShortestPath() {
        auto it = paths.begin();
        if (it == paths.end()) {
            return {{-1, -1}, {-1, -1}};
        }

        pair<pair<int, int>, pair<int, int>> min_element = *it;
        int taskId = min_element.first.first;

        removeUselessPaths(taskId);

        return min_element;
    }
public:
    pair<int, int> minCostMaxFlow() {
        int min_cost = 0, max_flow = 0;
        while (true) {
            if (pq.empty()) {
                break;
            }
            vector<int> currentPriority = pq[pq.size() - 1];
            bool flag = createPath(currentPriority);
            if (flag) {
                while (!paths.size()) {
                    pair<pair<int, int>, pair<int, int>> res = findShortestPath();
                    pair<pair<int, int>, pair<int, int>> target = {{-1, -1}, {-1, -1}};
                    if (res == target) {
                        break;
                    }
                    int taskId = res.first.first;
                    int nodeId = res.first.second;
                    int startTime = res.second.second;

                    if (taskId == -1) {
                        break;
                    }

                    min_cost += res.second.first;
                    max_flow++;

                    int tId = int(taskId / nTS);
                    assignedTasks[tasks[tId].name] = {nodes[nodeId].name, startTime};
                    setStartTime(tId, startTime + tasks[tId].duration);
                }
            } else {
                break;
            }
        }

        return {min_cost, max_flow};
    }
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
    for (auto &task_name : T) {
        for (auto &node_name : N) {
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

    map<pair<string, int>, int> capacity_per_time_slot;
    for (auto& [node, timeslot_capacity] : input["node_capacity_per_time"].items()) {
        for (auto& [time_slot, cap] : timeslot_capacity.items()) {
            int ts = stoi(time_slot);
            capacity_per_time_slot[{node, ts}] = cap;
        }
    }

    map<string, int> task_mp;
    for (int i = 0; i < T.size(); i++) {
        task_mp[T[i]] = i;
        cout << "Task: " << T[i] << ", id: " << task_mp[T[i]] << '\n';
    }

    vector<vector<int>> dependency_graph(T.size() + 1);
    vector<int> deg(T.size() + 1, 0);
    for (int i = 0; i < input["dependencies"].size(); i++) {
        string bef = input["dependencies"][i]["before"];
        string aft = input["dependencies"][i]["after"];
        int bef_id = task_mp[bef];
        int aft_id = task_mp[aft];
        deg[aft_id + 1]++;
        dependency_graph[bef_id + 1].push_back(aft_id + 1);
    }

    for (int i = 1; i <= T.size(); i++) {
        if (deg[i] == 0) {
            dependency_graph[0].push_back(i);
        }
    }

    vector<vector<int>> pq(T.size());
    vector<bool> visited(T.size() + 1, false);
    vector<int> priority(T.size() + 1);
    int idx = -1;
    auto bfs = [&](int root) {
        visited[root] = true;
        priority[root] = 0;
        queue<int> q;
        q.push(root);
        while (q.size()) {
            int v = q.front();
            q.pop();
            for (auto &x : dependency_graph[v]) {
                if (!visited[x] || (visited[x] && priority[x] < priority[v] + 1)) {
                    if (!visited[x]) {
                        visited[x] = true;
                    }
                    priority[x] = priority[v] + 1;
                    idx = max(idx, priority[x]);
                    pq[priority[x] - 1].push_back(x);
                    q.push(x);
                }
            }
        }
    };

    bfs(0);
    
    int ext = T.size() - idx;
    while (ext--) {
        pq.pop_back();
    }

    reverse(pq.begin(), pq.end());

    timeExpandedMCMF temcmf(timeStamps, pq, capacity_per_time_slot, dependency_graph);

    return 0;
}

