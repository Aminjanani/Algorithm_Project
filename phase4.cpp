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

    int idNumber = 0;
    vector<taskNode> T;
    vector<string> tasks;
    map<string, taskNode> taskMap;
    json assignedTasks = input["assigned_tasks"];
    for (int i = 0; i < (int)assignedTasks.size(); i++) {
        string id = assignedTasks[i]["id"];
        int cpu = assignedTasks[i]["cpu"];
        int ram = assignedTasks[i]["ram"];
        int duration = assignedTasks[i]["duration"];
        int deadline = assignedTasks[i]["deadline"];
        taskNode new_task = taskNode(id, cpu, ram, duration, deadline);
        T.push_back(new_task);
        tasks.push_back(id);
        taskMap[id] = new_task;
    }

    json resourcePerTime = input["resource_per_time"];
    map<int, int> timeSlotCap;
    for (auto& [slot, cap] : input["resource_per_time"].items()) {
        int time_slot = stoi(slot);
        int cpu_cap = cap["cpu"];
        timeSlotCap[time_slot] = cpu_cap;
    }

    vector<int> timeSlots = input["time_slot"];

    auto find_all_permutatoins = [&](vector<string>& tasks) -> vector<vector<taskNode>> {
        vector<vector<taskNode>> perms;
        sort(tasks.begin(), tasks.end());
        do {
            vector<taskNode> tmp;
            for (auto &x : tasks) {
                tmp.push_back(taskMap[x]);
            }
            perms.push_back(tmp);
        } while (next_permutation(tasks.begin(), tasks.end()));
    };

    auto fitTasks = [&](vector<vector<taskNode>>& taskPerms, int numOfTasks) {
        vector<vector<vector<int>>> dp;
        for (int i = 0; i < taskPerms.size(); i++) {
            for (int j = 0; j < T.size(); j++) {
                for (int k = 0; k < timeSlots.size(); k++) {

                }
            }
        }
    };

    return 0;
}
