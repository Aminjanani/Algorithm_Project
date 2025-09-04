#include <bits/stdc++.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

struct previous_schedule {
    string task;
    string node;
    int start_time;
    previous_schedule(string T, string N, int S) : task(T), node(N), start_time(S) {}
};

struct taskNode {
    int id;
    string name;
    int cpu;
    int ram;
    int deadline;
    map<int, int> exec_cost;

    taskNode(int ID, string NAME, int CPU, int RAM, int DEADLINE) {
        id = ID;
        name = NAME;
        cpu = CPU;
        ram = RAM;
        deadline = DEADLINE;
    }
};

struct event {
    string type;
    int time;
    string task;
    string node;
    int cpu;
    int ram;
    int deadline;

    event(string TYPE, int TIME, string TASK, string NODE, int CPU, int RAM, int DEADLINE) {
        type = TYPE;
        time = TIME;
        task = TASK;
        node = NODE;
        cpu = CPU;
        ram = RAM;
        deadline = DEADLINE;
    }
};


int main() {
    string file_name = "input3.json";
    ifstream file(file_name);
    if (!file.is_open()) {
        cout << "Cannot open the file: " << file_name << '\n';
        return 1;
    }
    json input;
    file >> input;

    vector<previous_schedule> schedules;
    for (const auto& item : input["previous_schedules"]) {
        schedules.emplace_back(item["task"], item["node"], item["start_time"]);
    }   
    
    vector<event> events;
    vector<taskNode> tasks;
    for (auto& item : input["events"]) {
        string type = item["type"];
        int time;
        if (item.contains("time")) {
            time = item["time"];
        } else {
            time = -1;
        }
        string task_name = "";
        string node = item.contains("node") ? item["node"] : "";
        int cpu = 0;
        int ram = 0;
        int deadline = 0;

        if (type == "node_failure") {
            time = item["time"];
            node = item["node"];
        } else if (type == "new_task") {
            auto t = item["task"];
            task_name = t["id"];
            cpu = t["cpu"];
            ram = t["ram"];
            deadline = t["deadline"];

            int task_id = stoi(task_name.substr(1));
            taskNode tn(task_id, task_name, cpu, ram, deadline);
            if (t.contains("exec_cost")) {
                for (auto& [node_str, cost] : t["exec_cost"].items()) {
                    int node_id = stoi(node_str.substr(1));
                    tn.exec_cost[node_id] = cost;
                }
            }
            tasks.push_back(tn);
        }
        events.emplace_back(type, time, task_name, node, cpu, ram, deadline);
    }

    

    map<string, map<int, int>> node_capacity_update;
    if (input.contains("node_capacity_update")) {
        for (auto& [node, updates] : input["node_capacity_update"].items()) {
            for (auto& [time_str, change] : updates.items()) {
                int time_val = stoi(time_str);
                node_capacity_update[node][time_val] = change;
            }
        }
    }

    return 0;
}
