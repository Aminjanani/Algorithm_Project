#include <bits/stdc++.h>
#include <optional>
#include "json.hpp"
using namespace std;

using json = nlohmann::json;

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

class timeExpendedMCMF {

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

    vector<int> timeStamps = input["time_slots"];

    map<map<string, int>, int> capacity_per_time_slot;

    map<string, string> dependencies;

    return 0;
}
