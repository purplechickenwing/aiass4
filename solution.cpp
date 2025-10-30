#define BN_LIB
#include "starter.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <iomanip>

using namespace std;

// Read data from file
vector<vector<string>> readData(const string& datafile, network& net) {
    vector<vector<string>> data;
    ifstream file(datafile);
    string line;
    
    int numVars = net.netSize();
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        vector<string> record;
        istringstream iss(line);
        string value;
        
        while (iss >> value) {
            // Remove quotes and commas
            value.erase(remove(value.begin(), value.end(), '\"'), value.end());
            value.erase(remove(value.begin(), value.end(), ','), value.end());
            record.push_back(value);
        }
        
        if (record.size() == numVars) {
            data.push_back(record);
        }
    }
    
    file.close();
    return data;
}

// Get CPT index for a given assignment
int getCPTIndex(list<Graph_Node>::iterator node, const vector<string>& assignment, network& net) {
    vector<string> parents = node->get_Parents();
    vector<string> nodeValues = node->get_values();
    int numValues = node->get_nvalues();
    
    // Get node index in assignment
    string nodeName = node->get_name();
    int nodeIdx = net.get_index(nodeName);
    string nodeValue = assignment[nodeIdx];
    
    // Find value index
    auto it = find(nodeValues.begin(), nodeValues.end(), nodeValue);
    if (it == nodeValues.end()) return -1;
    int valIdx = distance(nodeValues.begin(), it);
    
    int index = valIdx;
    int multiplier = numValues;
    
    // Add parent contributions (rightmost parent varies fastest)
    for (int i = parents.size() - 1; i >= 0; i--) {
        string parentName = parents[i];
        int parentIdx = net.get_index(parentName);
        string parentValue = assignment[parentIdx];
        
        auto parentNode = net.search_node(parentName);
        vector<string> parentValues = parentNode->get_values();
        
        auto pit = find(parentValues.begin(), parentValues.end(), parentValue);
        if (pit == parentValues.end()) return -1;
        int parentValIdx = distance(parentValues.begin(), pit);
        
        index += parentValIdx * multiplier;
        multiplier *= parentNode->get_nvalues();
    }
    
    return index;
}

// Learn parameters from data
void learnParameters(network& net, const string& datafile) {
    vector<vector<string>> data = readData(datafile, net);
    
    cout << "Loaded " << data.size() << " records" << endl;
    
    // Separate complete and incomplete data
    vector<vector<string>> completeData;
    vector<vector<string>> incompleteData;
    
    for (auto& record : data) {
        bool hasQuestion = false;
        for (auto& val : record) {
            if (val == "?") {
                hasQuestion = true;
                break;
            }
        }
        if (hasQuestion) {
            incompleteData.push_back(record);
        } else {
            completeData.push_back(record);
        }
    }
    
    cout << "Complete records: " << completeData.size() << endl;
    cout << "Incomplete records: " << incompleteData.size() << endl;
    
    // Process each node
    for (int i = 0; i < net.netSize(); i++) {
        auto node = net.get_nth_node(i);
        vector<float> cpt = node->get_CPT();
        
        // Check if this node needs learning
        bool needsLearning = false;
        for (float val : cpt) {
            if (val == -1) {
                needsLearning = true;
                break;
            }
        }
        
        if (!needsLearning) continue;
        
        // Initialize counts
        vector<double> counts(cpt.size(), 0.0);
        
        // Count from complete data
        for (auto& record : completeData) {
            int idx = getCPTIndex(node, record, net);
            if (idx >= 0 && idx < counts.size()) {
                counts[idx] += 1.0;
            }
        }
        
        // Handle incomplete data with uniform imputation
        for (auto& record : incompleteData) {
            // Find missing variable
            int missingIdx = -1;
            for (int j = 0; j < record.size(); j++) {
                if (record[j] == "?") {
                    missingIdx = j;
                    break;
                }
            }
            
            if (missingIdx == -1) continue;
            
            // Get missing variable's possible values
            auto missingNode = net.get_nth_node(missingIdx);
            vector<string> possibleValues = missingNode->get_values();
            
            // Try all possible values
            for (auto& possibleValue : possibleValues) {
                vector<string> filledRecord = record;
                filledRecord[missingIdx] = possibleValue;
                
                int idx = getCPTIndex(node, filledRecord, net);
                if (idx >= 0 && idx < counts.size()) {
                    counts[idx] += 1.0 / possibleValues.size();
                }
            }
        }
        
        // Normalize to get probabilities
        int numValues = node->get_nvalues();
        vector<string> parents = node->get_Parents();
        
        // Calculate parent configuration space size
        int parentConfigSize = 1;
        for (auto& parentName : parents) {
            auto parentNode = net.search_node(parentName);
            parentConfigSize *= parentNode->get_nvalues();
        }
        
        // Normalize each parent configuration
        vector<float> newCPT(cpt.size());
        
        for (int config = 0; config < parentConfigSize; config++) {
            double sum = 0.0;
            for (int val = 0; val < numValues; val++) {
                int idx = config * numValues + val;
                sum += counts[idx];
            }
            
            // Laplace smoothing
            for (int val = 0; val < numValues; val++) {
                int idx = config * numValues + val;
                if (sum > 0) {
                    newCPT[idx] = (counts[idx] + 1.0) / (sum + numValues);
                } else {
                    newCPT[idx] = 1.0 / numValues;
                }
            }
        }
        
        node->set_CPT(newCPT);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <network.bif> <data.dat>" << endl;
        return 1;
    }
    
    string networkFile = argv[1];
    string dataFile = argv[2];
    
    cout << "Reading network from " << networkFile << endl;
    network BayesNet = read_network(networkFile.c_str());
    
    cout << "Network loaded with " << BayesNet.netSize() << " nodes" << endl;
    
    cout << "Learning parameters from " << dataFile << endl;
    learnParameters(BayesNet, dataFile);
    
    cout << "Writing learned network to solved_hailfinder.bif" << endl;
    write_network("solved_hailfinder.bif", BayesNet);
    
    cout << "Learning complete!" << endl;
    
    return 0;
}