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

    taskNode() : id(0), name(""), cpu(0), ram(0), deadline(0) {}
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

struct scheduleDetails {
    string node;
    int start_time;
    bool meets_deadline;
    scheduleDetails() : node(""), start_time(-1), meets_deadline(false) {}
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
    for (const auto& [task, details] : input["previous_schedule"].items()) {
        schedules.emplace_back(task, details["node"], details["start_time"]);
    }   
    
    vector<event> events;
    vector<taskNode> tasks;
    set<string> task_names;
    for (auto& item : input["events"]) {
        string type = item["type"];
        int time = item.contains("time") ? static_cast<int>(item["time"]) : -1;
        string task_name = "";
        string node = item.contains("node") ? item["node"] : "";
        int cpu = 0;
        int ram = 0;
        int deadline = 0;

        if (type == "new_task") {
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
            task_names.insert(task_name);
        } else if (type == "node_failure") {
            node = item["node"];
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

    set<string> nodes;
    set<int> time_slots;
    for (const auto& s : schedules) {
        nodes.insert(s.node);
        time_slots.insert(s.start_time);
    }
    for (const auto& e : events) {
        if (e.type == "node_failure") nodes.insert(e.node);
        if (e.time != -1) time_slots.insert(e.time);
    }
    for (const auto& [node, updates] : node_capacity_update) {
        nodes.insert(node);
        for (const auto& [time, _] : updates) {
            time_slots.insert(time);
        }
    }

    for (const auto& s : schedules) {
        task_names.insert(s.task);
        int task_id = stoi(s.task.substr(1));
        taskNode tn(task_id, s.task, 1, 1, s.start_time + 2); 
        for (const auto& node : nodes) {
            int node_id = stoi(node.substr(1));
            tn.exec_cost[node_id] = 1; // Default exec_cost
        }
        tasks.push_back(tn);
    }
    vector<string> all_tasks(task_names.begin(), task_names.end());

    // Initialize node capacities
    map<string, map<int, int>> node_cpu_capacity;
    for (const auto& node : nodes) {
        for (const auto& time : time_slots) {
            node_cpu_capacity[node][time] = 2; // Default capacity
        }
    }
    for (const auto& [node, updates] : node_capacity_update) {
        for (const auto& [time, cap] : updates) {
            node_cpu_capacity[node][time] = cap;
        }
    }
    for (const auto& e : events) {
        if (e.type == "node_failure") {
            for (int t = e.time; t <= *time_slots.rbegin(); t++) {
                node_cpu_capacity[e.node][t] = 0;
            }
        }
    }

    // Initialize scheduling details
    map<string, scheduleDetails> scheduling_details;
    for (const auto& task : all_tasks) {
        scheduling_details[task] = scheduleDetails();
    }

    // Get all subsets of tasks
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

    map<string, int> task_cpu, task_deadline, task_duration;
    map<string, map<string, int>> task_exec_cost;
    for (const auto& t : tasks) {
        task_cpu[t.name] = t.cpu;
        task_deadline[t.name] = t.deadline;
        task_duration[t.name] = 1; 
        for (const auto& [node_id, cost] : t.exec_cost) {
            task_exec_cost[t.name]["N" + to_string(node_id)] = cost;
        }
    }
 
    for (const auto& item : input["events"]) {
        if (item["type"] == "new_task") {
            auto t = item["task"];
            string task_name = t["id"];
            if (t.contains("duration")) {
                task_duration[task_name] = t["duration"];
            }
        }
    }

    map<string, pair<string, int>> prev_schedule;
    for (const auto& s : schedules) {
        prev_schedule[s.task] = {s.node, s.start_time};
    }

    // fit tasks into time slots using dynamic programming
    // dp definition : dp[time_slot][sub_task_k][node_n] : minimum cost of doing sub_task_k until time_slot 
    // so the last sub_task(which is a subset of sub_task_k itself) is done on node_n
    auto fitTasks = [&]() {
        set<string> Tasks(all_tasks.begin(), all_tasks.end());
        vector<set<string>> allSubsets = getAllSubsets(Tasks);
        map<tuple<int, string, set<string>>, long long> dp;
        map<tuple<int, string, set<string>>, tuple<int, string, set<string>>> parent;

        for (const auto& node : nodes) {
            for (int j = 0; j < allSubsets.size(); j++) {
                auto& subset = allSubsets[j];
                int total_cpu = 0;
                bool valid = true;
                for (const auto& task : subset) {
                    if (!task_cpu.count(task) || !task_deadline.count(task) || !task_exec_cost[task].count(node)) {
                        valid = false;
                        break;
                    }
                    total_cpu += task_cpu[task];
                    if (task_deadline[task] < 1) {
                        valid = false;
                    }
                }
                if (valid && total_cpu <= node_cpu_capacity[node][0]) {
                    long long cost = 0;
                    for (const auto& task : subset) {
                        cost += task_exec_cost[task][node]; // Only exec_cost
                    }
                    dp[{0, node, subset}] = cost;
                } else {
                    dp[{0, node, subset}] = INT_MAX;
                }
                parent[{0, node, subset}] = {-1, "", {}};
            }
        }

        for (int t : time_slots) {
            if (t == 0) continue;
            for (const auto& node : nodes) {
                for (int j = 0; j < allSubsets.size(); j++) {
                    auto& subset = allSubsets[j];
                    dp[{t, node, subset}] = INT_MAX;
                    vector<set<string>> subsubsets = getAllSubsets(subset);
                    for (const auto& subsubset : subsubsets) {
                        set<string> diff_set;
                        set_difference(subset.begin(), subset.end(), subsubset.begin(), subsubset.end(),
                                       inserter(diff_set, diff_set.begin()));
                        int total_cpu = 0;
                        bool valid = true;
                        for (const auto& task : subsubset) {
                            if (!task_cpu.count(task) || !task_deadline.count(task) || !task_exec_cost[task].count(node)) {
                                valid = false;
                                break;
                            }
                            total_cpu += task_cpu[task];
                            if (task_deadline[task] < t + task_duration[task]) {
                                valid = false;
                            }
                        }
                        if (!valid || total_cpu > node_cpu_capacity[node][t]) {
                            continue;
                        }
                        for (const auto& prev_node : nodes) {
                            long long prev_cost = dp[{t - 1, prev_node, diff_set}];
                            if (prev_cost != INT_MAX) {
                                long long cost = prev_cost;
                                for (const auto& task : subsubset) {
                                    cost += task_exec_cost[task][node]; 
                                }
                                if (dp[{t, node, subset}] > cost) {
                                    dp[{t, node, subset}] = cost;
                                    parent[{t, node, subset}] = {t - 1, prev_node, diff_set};
                                }
                            }
                        }
                        
                        if (subsubset.empty()) {
                            for (const auto& prev_node : nodes) {
                                long long prev_cost = dp[{t - 1, prev_node, subset}];
                                if (prev_cost != INT_MAX && dp[{t, node, subset}] > prev_cost) {
                                    dp[{t, node, subset}] = prev_cost;
                                    parent[{t, node, subset}] = {t - 1, prev_node, subset};
                                }
                            }
                        }
                    }
                }
            }
        }

        int opt_time = -1;
        string opt_node;
        set<string> opt_subset;
        long long min_cost = INT_MAX;
        for (int t : time_slots) {
            for (const auto& node : nodes) {
                for (int j = 0; j < allSubsets.size(); j++) {
                    if (dp[{t, node, allSubsets[j]}] < min_cost || 
                        (dp[{t, node, allSubsets[j]}] == min_cost && allSubsets[j].size() > opt_subset.size())) {
                        min_cost = dp[{t, node, allSubsets[j]}];
                        opt_time = t;
                        opt_node = node;
                        opt_subset = allSubsets[j];
                    }
                }
            }
        }

        int done_tasks = 0;
        set<string> scheduled_tasks;
        map<int, map<string, set<string>>> assignments;
        if (opt_time != -1) {
            auto curr = make_tuple(opt_time, opt_node, opt_subset);
            while (opt_time != -1) {
                auto [prev_time, prev_node, prev_subset] = parent[curr];
                set<string> tasks_scheduled;
                set_difference(opt_subset.begin(), opt_subset.end(), prev_subset.begin(), prev_subset.end(),
                               inserter(tasks_scheduled, tasks_scheduled.begin()));
                for (const auto& task : tasks_scheduled) {
                    assignments[opt_time][opt_node].insert(task);
                    scheduled_tasks.insert(task);
                    scheduling_details[task].node = opt_node;
                    scheduling_details[task].start_time = opt_time;
                    scheduling_details[task].meets_deadline = opt_time + task_duration[task] <= task_deadline[task];
                    done_tasks++;
                }
                opt_subset = prev_subset;
                opt_time = prev_time;
                opt_node = prev_node;
                curr = {opt_time, opt_node, opt_subset};
            }
        }

        vector<string> remaining_tasks;
        for (const auto& task : all_tasks) {
            if (scheduling_details[task].start_time == -1) {
                remaining_tasks.push_back(task);
            }
        }
        sort(remaining_tasks.begin(), remaining_tasks.end(),
             [&](const string& a, const string& b) {
                 return task_deadline[a] < task_deadline[b];
             });

        map<int, map<string, int>> used_cpu;
        for (const auto& task : scheduled_tasks) {
            if (scheduling_details[task].start_time != -1) {
                used_cpu[scheduling_details[task].start_time][scheduling_details[task].node] += task_cpu[task];
            }
        }

        for (const auto& task : remaining_tasks) {
            bool assigned = false;
            for (int t : time_slots) {
                for (const auto& node : nodes) {
                    if (task_exec_cost[task].count(node) && t + task_duration[task] <= task_deadline[task]) {
                        int current_cpu = used_cpu[t][node];
                        if (current_cpu + task_cpu[task] <= node_cpu_capacity[node][t]) {
                            // Prefer original time and node for previous tasks
                            if (prev_schedule.count(task) && prev_schedule[task].first == node && prev_schedule[task].second == t) {
                                scheduling_details[task].node = node;
                                scheduling_details[task].start_time = t;
                                scheduling_details[task].meets_deadline = true;
                                used_cpu[t][node] += task_cpu[task];
                                scheduled_tasks.insert(task);
                                done_tasks++;
                                assigned = true;
                                break;
                            } else if (!prev_schedule.count(task) || prev_schedule[task].first != node || prev_schedule[task].second != t) {
                                scheduling_details[task].node = node;
                                scheduling_details[task].start_time = t;
                                scheduling_details[task].meets_deadline = true;
                                used_cpu[t][node] += task_cpu[task];
                                scheduled_tasks.insert(task);
                                done_tasks++;
                                assigned = true;
                                break;
                            }
                        }
                    }
                }
                if (assigned) break;
            }
        }

        int change_count = 0;
        for (const auto& task : all_tasks) {
            if (prev_schedule.count(task) && (scheduling_details[task].start_time != prev_schedule[task].second ||
                                              scheduling_details[task].node != prev_schedule[task].first)) {
                change_count++;
            } else if (task_names.count(task) && scheduling_details[task].start_time != -1) {
                change_count++;
            }
        }

        json output;
        output["updated_schedule"] = json::object();
        for (const auto& [task, details] : scheduling_details) {
            if (details.start_time != -1) {
                output["updated_schedule"][task] = {
                    {"node", details.node},
                    {"start_time", details.start_time}
                };
            }
        }
        output["reassigned_tasks"] = json::array();
        for (const auto& task : all_tasks) {
            if (prev_schedule.count(task) && (scheduling_details[task].start_time != prev_schedule[task].second ||
                                              scheduling_details[task].node != prev_schedule[task].first)) {
                output["reassigned_tasks"].push_back(task);
            } else if (task_names.count(task) && scheduling_details[task].start_time != -1) {
                output["reassigned_tasks"].push_back(task);
            }
        }
        output["failed_tasks"] = json::array();
        for (const auto& task : all_tasks) {
            if (scheduling_details[task].start_time == -1) {
                output["failed_tasks"].push_back(task);
            }
        }
        output["total_cost"] = min_cost + 2 * (all_tasks.size() - done_tasks);
        output["change_penalty"] = 2;
        cout << output.dump(2) << endl;
    };

    fitTasks();

    return 0;
}
