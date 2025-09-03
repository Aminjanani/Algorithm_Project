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

    taskNode() : name(""), cpu(0), ram(0), duration(0), deadline(0) {}
    taskNode(string NAME, int CPU, int RAM, int DURATION, int DEADLINE) {
        name = NAME;
        cpu = CPU;
        ram = RAM;
        duration = DURATION;  
        deadline = DEADLINE;
    }
};

struct taskDetails {
    int start_time;
    bool meet_deadline;
    taskDetails() : start_time(-1), meet_deadline(false) {}
};

int main() {
    string file_name = "input4.json";
    ifstream file(file_name);
    if (!file.is_open()) {
        cout << "Error: Could not open input file!" << '\n';
        return 1;
    }
    json input;
    file >> input;

    string node = input["node"];

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

    vector<int> timeSlots = input["time_slots"];

    // penalty factor
    int penalty = 2;

    map<string, taskDetails> schedulingDetails;
    for (int i = 0; i < tasks.size(); i++) {
        schedulingDetails[tasks[i]] = taskDetails();
    }

    // compute all possible subset of the original tasks
    auto getAllSubsets = [&](const set<string>& s) -> vector<set<string>> {
        int n = s.size();
        vector<string> elements(s.begin(), s.end());
        vector<set<string>> subsets;
        for (int mask = 0; mask < (1 << n); mask++) {
            set<string> subset;
            for (int i = 0; i < n; i++) {
                if (mask & (1 << i)) {
                    subset.insert(elements[i]); 
                }
            }
            subsets.push_back(subset);
        }
        return subsets;
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
        set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(diff_set, diff_set.begin()));
        return diff_set;
    };

    map<pair<int, set<string>>, pair<int, set<string>>> parent;

    // fit tasks into time slots using dynamic programming
    auto fitTasks = [&]() {
        set<string> Tasks(tasks.begin(), tasks.end());
        vector<set<string>> allSubsets = getAllSubsets(Tasks);
        map<pair<int, set<string>>, long long> dp;
        
        for (int j = 0; j < allSubsets.size(); j++) {
            auto [max_start_time, total_cpu] = getMaxStartTime(allSubsets[j]);
            if (max_start_time >= 0 && total_cpu <= timeSlotCap[0]) {
                dp[{0, allSubsets[j]}] = 0;
            } else {
                dp[{0, allSubsets[j]}] = INT_MAX;
            }
            parent[{0, allSubsets[j]}] = {-1, {}};
        }
        
        for (int i = 1; i < timeSlots.size(); i++) {
            for (int j = 0; j < allSubsets.size(); j++) {
                dp[{i, allSubsets[j]}] = INT_MAX;
                vector<set<string>> subsubsets = getAllSubsets(allSubsets[j]);
                for (int k = 0; k < subsubsets.size(); k++) {
                    set<string> diff_set = diff(allSubsets[j], subsubsets[k]);
                    auto [max_start_time, total_cpu] = getMaxStartTime(subsubsets[k]);
                    if (subsubsets[k].empty()) {
                        if (dp[{i - 1, diff_set}] != INT_MAX && dp[{i, allSubsets[j]}] > dp[{i - 1, diff_set}] + penalty) {
                            dp[{i, allSubsets[j]}] = dp[{i - 1, diff_set}] + penalty;
                            parent[{i, allSubsets[j]}] = {i - 1, diff_set};
                        }
                    } else if (max_start_time >= i && total_cpu <= timeSlotCap[i]) {
                        if (dp[{i - 1, diff_set}] != INT_MAX && dp[{i, allSubsets[j]}] > dp[{i - 1, diff_set}]) {
                            dp[{i, allSubsets[j]}] = dp[{i - 1, diff_set}];
                            parent[{i, allSubsets[j]}] = {i - 1, diff_set};
                        }
                    }
                }
            }
        }

        int idx = -1, maxDoneSubTasksId = -1;
        long long min_cost = INT_MAX;
        for (int j = 0; j < allSubsets.size(); j++) {
            if (allSubsets[j].size() == Tasks.size()) {
                for (int i = 0; i < timeSlots.size(); i++) {
                    if (dp[{i, allSubsets[j]}] != INT_MAX && min_cost > dp[{i, allSubsets[j]}]) {
                        min_cost = dp[{i, allSubsets[j]}];
                        idx = i;
                        maxDoneSubTasksId = j;
                    }
                }
            }
        }

        int done_tasks = 0;
        vector<int> used_cpu(timeSlots.size(), 0);
        if (idx != -1 && maxDoneSubTasksId != -1) {
            set<string> curr_subset = allSubsets[maxDoneSubTasksId];
            while (idx != -1) {
                set<string> prev_subset = parent[{idx, curr_subset}].second;
                set<string> scheduled_tasks = diff(curr_subset, prev_subset);
                for (const auto& task : scheduled_tasks) {
                    if (used_cpu[idx] + taskCpuMap[task] <= timeSlotCap[idx]) {
                        schedulingDetails[task].start_time = idx;
                        schedulingDetails[task].meet_deadline = idx + taskConstMap[task].first <= taskConstMap[task].second;
                        used_cpu[idx] += taskCpuMap[task];
                        done_tasks++;
                    }
                }
                curr_subset = prev_subset;
                idx = parent[{idx, curr_subset}].first;
            }
        }

        vector<pair<string, taskNode>> remaining_tasks;
        for (const auto& task : tasks) {
            if (schedulingDetails[task].start_time == -1) {
                remaining_tasks.push_back({task, taskMap[task]});
            }
        }
        sort(remaining_tasks.begin(), remaining_tasks.end(), 
             [&](const auto& a, const auto& b) {
                 return taskConstMap[a.first].second < taskConstMap[b.first].second;
             });

        for (const auto& [task_id, task] : remaining_tasks) {
            for (int i = 0; i < timeSlots.size(); i++) {
                if (used_cpu[i] + task.cpu <= timeSlotCap[i] && i + task.duration <= task.deadline) {
                    schedulingDetails[task_id].start_time = i;
                    schedulingDetails[task_id].meet_deadline = true;
                    used_cpu[i] += task.cpu;
                    done_tasks++;
                    break;
                }
            }
        }

        json output;
        output["execution_schedule"] = json::object();
        for (const auto& [task_id, details] : schedulingDetails) {
            output["execution_schedule"][task_id] = {
                {"start_time", details.start_time},
                {"meets_deadline", details.meet_deadline}
            };
        }

        output["total_idle_time"] = 0;
        output["penalty_cost"] = penalty * (T.size() - done_tasks);

        string outputFileName = "phase4_output.json";
        ofstream ofile(outputFileName);
        if (ofile.is_open()) {
            ofile << output.dump(2) << '\n';
            ofile.close();
        } else {
            cout << "Error: Could not open output file!" << '\n';
        }
        cout << output.dump(2) << '\n';
    };

    fitTasks();

    return 0;
}
