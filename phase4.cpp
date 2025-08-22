#include <bits/stdc++.h>
#include <optional>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

struct taskNode {
    string name;
    int cpu;
    int ram;
    int duration;
    int deadline;

    taskNode(string NAME, int CPU, int RAM, int DURATION, int DEADLINE) {
        name = NAME;
        cpu = CPU;
        ram = RAM;
        duration = DURATION;
        deadline = DEADLINE;
    }
};

int main() {
    string file_name = "input4.json";
    ifstream file(file_name);
    json input;
    file >> input;

    string node = input["node"];

    vector<taskNode> T;
    json assignedTasks = input["assigned_tasks"];
    for (int i = 0; i < (int)assignedTasks.size(); i++) {
        string id = assignedTasks[i]["id"];
        int cpu = assignedTasks[i]["cpu"];
        int ram = assignedTasks[i]["ram"];
        int duration = assignedTasks[i]["duration"];
        int deadline = assignedTasks[i]["deadline"];
        T.push_back(taskNode(id, cpu, ram, duration, deadline));
    }

    json resourcePerTime = input["resource_per_time"];
    map<int, int> timeSlotCap;
    for (auto& [slot, cap] : input["resource_per_time"].items()) {
        int time_slot = stoi(slot);
        int cpu_cap = cap["cpu"];
        timeSlotCap[time_slot] = cpu_cap;
    }

    vector<int> timeSlots = input["time_slot"];

    auto fitTasks = [&]() {
        
    };

    return 0;
}
