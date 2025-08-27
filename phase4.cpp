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
    map<string, pair<int, int>> taskConstMap;
    map<string, int> taskCpuMap;
    json assignedTasks = input["assigned_tasks"];
    for (int i = 0; i < (int)assignedTasks.size(); i++) {
        string id = assignedTasks[i]["id"];
        int cpu = assignedTasks[i]["cpu"];
        int ram = assignedTasks[i]["ram"];
        int duration = assignedTasks[i]["duration"];
        int deadline = assignedTasks[i]["deadline"];
        taskConstMap[id] = {duration, deadline};
        taskCpuMap[id] = cpu;
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

    // compute all possible subset of the original tasks
    auto getAllSubsets = [&](const set<string>& s) -> vector<set<string>> {
        int n = s.size();
        vector<string> elements(s.begin(), s.end());
        vector<set<string>> subSets;
        for (int mask = 0; mask < (1 << n); mask++) {
            set<string> subset;
            for (int i = 0; i < n; i++) {
                if (mask && (1 << i)) {
                    subset.insert(elements[i]); 
                }
            }
            subSets.push_back(subset);
        }
        return subSets;
    };

    // compute the maximum possible start time and total cpu for a subset of tasks
    auto getMaxStartTime = [&](set<string>& s) -> pair<int, int> {
        int max_start = -1, total_cpu = 0;
        for (auto &x : s) {
            max_start = max(max_start, taskConstMap[x].second - taskConstMap[x].first);
            total_cpu += taskCpuMap[x];
        }

        return {max_start, total_cpu};
    };

    // compute the difference between two sets of tasks
    auto diff = [&](set<string>& s1, set<string>& s2) -> set<string> {
        set<string> diff_set;
        set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), back_inserter(diff_set));
        return diff_set;
    };

    map<pair<int, set<string>>, pair<int, set<string>>> parent;

    // fit tasks into time slots using dynamic programming
    auto fitTasks = [&]() {
        set<string> Tasks;
        for (int i = 0; i < tasks.size(); i++) {
            Tasks.insert(tasks[i]);
        }

        vector<set<string>> allSubsets = getAllSubsets(Tasks);
        map<pair<int, set<string>>, long int> dp;
        for (int j = 0; j < allSubsets.size(); j++) {
            pair<int, int> target = getMaxStartTime(allSubsets[j]);
            if (target.first >= 0 && target.second <= timeSlotCap[0]) {
                dp[{0, allSubsets[j]}] = 0;
            } else {
                dp[{0, allSubsets[j]}] = -1;
            }
        }
        for (int i = 1; i < timeSlots.size(); i++) {
            for (int j = 0; j < allSubsets.size(); j++) {
                dp[{i, allSubsets[j]}] = INT_MAX;
            }
        }

        for (int i = 1; i < timeSlots.size(); i++) {
            for (int j = 0; j < allSubsets.size(); j++) {
                vector<set<string>> subsubset = getAllSubsets(allSubsets[j]);
                for (int k = 0; k < subsubset.size(); k++) {
                    set<string> diff_set = diff(allSubsets[j], subsubset[k]);
                    if (subsubset[k].size() == 0) {
                        dp[{i, allSubsets[j]}] = dp[{i - 1, diff_set}] + 1;
                        parent[{i, allSubsets[j]}] = {i - 1, diff_set};
                    } else {
                        pair<int, int> target = getMaxStartTime(subsubset[k]);
                        if (target.first >= i && target.second <= timeSlotCap[i]) {
                            if (dp[{i, allSubsets[j]}] > dp[{i - 1, diff_set}]) {
                                dp[{i, allSubsets[j]}] = dp[{i - 1, diff_set}];
                                parent[{i, allSubsets[j]}] = {i - 1, diff_set};
                            }
                        }
                    }
                }
            }
        }
    };

    return 0;
}
