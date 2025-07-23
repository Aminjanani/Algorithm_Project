#include <bits/stdc++.h>
#include "json.hpp"
using namespace std;

using json = nlohmann::json;

int main() {
    string file_name = "input2.json";
    ifstream file(file_name);
    json input;
    file >> input;

    return 0;
}