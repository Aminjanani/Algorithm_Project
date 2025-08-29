#include <bits/stdc++.h>
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

    taskNode(int ID, string NAME, int CPU, int RAM, int DEADLINE, int DURATION)
        : id(ID), name(NAME), cpu(CPU), ram(RAM), deadline(DEADLINE), duration(DURATION) {}
};

struct Node {
    int id;
    string name;
    int cpu_cap;
    int ram_cap;

    Node(int ID, string NAME, int CPU_CAP, int RAM_CAP)
        : id(ID), name(NAME), cpu_cap(CPU_CAP), ram_cap(RAM_CAP) {}
};

class timeExpandedMCMF {
private:
    vector<taskNode> tasks;
    vector<Node> nodes;
    map<pair<string,string>, int> exec_cost; 
    vector<int> timeSlots;                  
    vector<vector<int>> layers;     
    map<pair<string,int>, int> startCap;    
    vector<vector<int>> dep_graph_raw;      
    vector<vector<int>> dep_graph;         
    int T = 0, N = 0, TS = 0;
    vector<int> earliestStart;  
    vector<int> latestStart;    
    vector<int> finishTime;      
    vector<bool> assigned;       
    vector<vector<int>> used_cpu;  
    vector<vector<int>> used_ram;  
    map<pair<string,int>, int> starts_used; 
    map<string, pair<string,int>> assignment; 

    using Item = tuple<int,int,int,int>;
    set<Item> candidates;

public:
    timeExpandedMCMF(const vector<taskNode>& TASKS,
                     const vector<Node>& NODES,
                     const map<pair<string,string>, int>& COSTS,
                     const vector<int>& TSLOTS,
                     const vector<vector<int>>& PQ,
                     const map<pair<string,int>, int>& capacity_per_time_slot,
                     const vector<vector<int>>& dep_from_main)
        : tasks(TASKS), nodes(NODES), exec_cost(COSTS),
          timeSlots(TSLOTS), layers(PQ), startCap(capacity_per_time_slot),
          dep_graph_raw(dep_from_main) {

        T  = (int)tasks.size();
        N  = (int)nodes.size();
        TS = (int)timeSlots.size();

        earliestStart.assign(T, 0);
        latestStart.assign(T, 0);
        finishTime.assign(T, -1);
        assigned.assign(T, false);
        dep_graph.assign(T + 1, vector<int>());

        used_cpu.assign(N, vector<int>(max(TS, 1), 0));
        used_ram.assign(N, vector<int>(max(TS, 1), 0));

        // Initialize dep_graph based on dep_graph_raw
        for (int i = 0; i < dep_graph_raw.size(); i++) {
            for (int j : dep_graph_raw[i]) {
                if (j > 0 && j <= T) {
                    dep_graph[i].push_back(j - 1); // Adjust to 0-based indexing
                }
            }
        }

        for (int i = 0; i < T; i++) {
            latestStart[i] = tasks[i].deadline - tasks[i].duration;
        }
    }

private:
    int start_capacity_safe(const string& nodeName, int start) const {
        auto it = startCap.find({nodeName, start});
        if (it == startCap.end()) return INT_MAX; 
        return it->second;
    }

    int starts_used_safe(const string& nodeName, int start) const {
        auto it = starts_used.find({nodeName, start});
        return (it == starts_used.end() ? 0 : it->second);
    }

    bool feasible_on_node_time(int taskId, int nodeId, int start) {
        if (taskId < 0 || taskId >= T || nodeId < 0 || nodeId >= N) {
            cerr << "[feasible] OOB taskId=" << taskId << " nodeId=" << nodeId << "\n";
            return false;
        }
        if (TS == 0) {
            return false;
        }

        const auto &tk = tasks[taskId];
        const auto &nd = nodes[nodeId];

        if (start < 0) {
            return false;
        }
        if (tk.duration <= 0) {
            return false;
        }
        if (start + tk.duration > TS) {
            return false;
        }
        if (start < earliestStart[taskId] || start > latestStart[taskId]) {
            return false;
        }

        int capStarts = start_capacity_safe(nd.name, start);
        int usedStarts = starts_used_safe(nd.name, start);
        if (usedStarts + 1 > capStarts) {
            return false;
        }

        for (int t = start; t < start + tk.duration; t++) {
            if (t < 0 || t >= TS) {
                return false;
            }
            auto it = startCap.find({nd.name, t});
            int node_cpu_cap = (it != startCap.end() ? it->second : INT_MAX);
            if (used_cpu[nodeId][t] + tk.cpu > node_cpu_cap) {
                return false;
            }
        }
        return true;
    }

    void add_candidates_for_layer(const vector<int>& layerTasks) {
        for (int taskId : layerTasks) {
            if (taskId < 0 || taskId >= T) {
                cerr << "[add_cand] bad taskId=" << taskId << "\n";
                continue;
            }
            if (assigned[taskId]) continue;

            int est = max(0, earliestStart[taskId]);
            int lst = latestStart[taskId];
            if (lst < est) continue;

            for (int nodeId = 0; nodeId < N; nodeId++) {
                auto itc = exec_cost.find({tasks[taskId].name, nodes[nodeId].name});
                int c = (itc == exec_cost.end() ? INF : itc->second);
                if (c >= INF) continue;

                for (int start = est; start <= lst; start++) {
                    if (feasible_on_node_time(taskId, nodeId, start)) {
                        candidates.emplace(c, start, taskId, nodeId);
                    }
                }
            }
        }
    }

    void assign_choice(const Item& it, int& total_cost, int& total_flow) {
        auto [c, start, taskId, nodeId] = it;

        if (!feasible_on_node_time(taskId, nodeId, start)) {
            return;
        }

        for (int t = start; t < start + tasks[taskId].duration; t++) {
            used_cpu[nodeId][t] += tasks[taskId].cpu;
            used_ram[nodeId][t] += tasks[taskId].ram;
        }
        starts_used[{nodes[nodeId].name, start}] += 1;

        assigned[taskId] = true;
        finishTime[taskId] = start + tasks[taskId].duration;
        assignment[tasks[taskId].name] = {nodes[nodeId].name, start};

        for (int v : dep_graph[taskId + 1]) {
            if (v >= 0 && v < T) {
                earliestStart[v] = max(earliestStart[v], finishTime[taskId]);
            }
        }

        total_cost += c;
        total_flow += 1;
    }

public:
    pair<int,int> minCostMaxFlow() {
        int min_cost = 0, max_flow = 0;
        for (int li = 0; li < (int)layers.size(); li++) {
            const auto& layerTasks = layers[li];

            candidates.clear();
            add_candidates_for_layer(layerTasks);

            while (!candidates.empty()) {
                auto it = candidates.begin();
                Item best = *it;
                candidates.erase(it);

                int beforeFlow = max_flow;
                assign_choice(best, min_cost, max_flow);

                if (max_flow != beforeFlow) {
                    candidates.clear();
                    add_candidates_for_layer(layerTasks);
                }
            }
        }
        return {min_cost, max_flow};
    }

    void print_result_json() {
        json out;
        json sched = json::object();
        for (auto &kv : assignment) {
            const string& taskName = kv.first;
            const string& nodeName = kv.second.first;
            int start = kv.second.second;
            sched[taskName] = { {"node", nodeName}, {"start_time", start} };
        }
        out["schedule"] = sched;

        int total_cost = 0;
        for (auto &kv : assignment) {
            auto itc = exec_cost.find({kv.first, kv.second.first});
            if (itc != exec_cost.end()) {
                total_cost += itc->second;
            }
        }
        
        out["total_cost"] = total_cost;

        bool valid = ((int)assignment.size() == T);
        out["valid"] = valid;

        cout << out.dump(2) << "\n";

        string outputFileName = "phase2_output.json";
        ofstream ofile(outputFileName);
        if (ofile.is_open()) {
            ofile << out.dump(2) << "\n";
            ofile.close();
        } else {
            cerr << "Error: Could not open output file!" << endl;
        }
    }
};

int main() {
    string file_name = "input2.json";
    ifstream file(file_name);
    if (!file.is_open()) {
        cerr << "Error: Could not open input file!" << endl;
        return 1;
    }
    json input;
    file >> input;
    file.close();

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

    json nodes_arr = input["nodes"];
    vector<Node> nodes;
    for (int i = 0; i < nodes_arr.size(); i++) {
        string name = nodes_arr[i]["id"];
        N.push_back(name);
        int cpu_cap = nodes_arr[i]["cpu_capacity"];
        int ram_cap = nodes_arr[i]["ram_capacity"];
        Node nd(i, name, cpu_cap, ram_cap);
        nodes.push_back(nd);
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
                    pq[priority[x] - 1].push_back(x - 1); 
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

    timeExpandedMCMF temcmf(taskNodes, nodes, exec_cost_mp, timeStamps, pq, capacity_per_time_slot, dependency_graph);

    auto [min_cost, max_flow] = temcmf.minCostMaxFlow();
    cout << "Flow: " << max_flow << ", Cost: " << min_cost << "\n";
    temcmf.print_result_json();

    return 0;
}
