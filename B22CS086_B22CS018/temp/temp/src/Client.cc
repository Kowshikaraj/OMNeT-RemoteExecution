#include <omnetpp.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <map>

using namespace omnetpp;
using namespace std;

const string OUTPUT_FILE = "output.txt";

class Client : public cSimpleModule {
private:
    // Parameters
    int arraySize;
    int numSubtasks;
    int numServers;
    int numClients;

    // Task data
    vector<int> dataArray;
    vector<vector<int>> subtasks;

    // For tracking results
    unordered_map<int, vector<pair<int, int>>> subtaskResults; // subtaskId -> [(serverId, result)]
    unordered_map<int, int> majorityResults; // subtaskId -> majority result
    int finalResult;
    int currentTaskId; // Track the current task ID
    int tasksCompleted;

    // Modified Server scores tracking
    struct ServerTrackingInfo {
        int score;          // Total correct results
        int subtaskCount;   // Total subtasks given to this server

        ServerTrackingInfo() : score(0), subtaskCount(0) {}
    };

    // Map to track scores and subtask counts for each server
    unordered_map<int, ServerTrackingInfo> serverTracking;

    // Map to track aggregated scores and subtask counts from all clients
    unordered_map<int, ServerTrackingInfo> aggregatedServerTracking;

    // Map to maintain average scores for server selection
    unordered_map<int, double> serverAvgScores;

    // For gossip protocol
    unordered_map<string, bool> messageLog;

    // Random number generator
    mt19937 rng;

protected:
    virtual void initialize() override {
        // Initialize random number generator
        rng.seed(static_cast<unsigned int>(time(nullptr)) + getIndex());

        // Get parameters from NED file
        arraySize = par("arraySize");
        numSubtasks = par("numSubtasks");
        numServers = par("numServers");
        numClients = par("numClients");

        // Initialize server tracking structures
        for (int i = 0; i < numServers; i++) {
            serverTracking[i] = ServerTrackingInfo();
            aggregatedServerTracking[i] = ServerTrackingInfo();
            serverAvgScores[i] = 0.0;
        }

        tasksCompleted = 0;
        currentTaskId = 0; // Initialize task ID

        // Clear output file for this run
        if (getIndex() == 0) {
            ofstream outFile(OUTPUT_FILE, ios::trunc);
            outFile.close();
        }

        // Schedule task execution
        scheduleAt(simTime() + 1.0, new cMessage("StartTask"));
    }

    virtual void handleMessage(cMessage *msg) override {
        if (strcmp(msg->getName(), "StartTask") == 0) {
            // Start a new task
            startTask();
            delete msg;
        }
        else if (strcmp(msg->getName(), "ResultMessage") == 0) {
            // Handle result from server
            handleResultMessage(msg);
        }
        else if (strcmp(msg->getName(), "GossipMessage") == 0) {
            // Handle gossip message
            handleGossipMessage(msg);
        }
        else {
            delete msg;
        }
    }

    void startTask() {
        // Increment task ID for a new task
        currentTaskId = tasksCompleted + 1; // Tasks are 1-indexed

        // Generate data array with random integers
        generateDataArray();

        // Divide task into subtasks
        divideIntoSubtasks();

        // Log task start
        logToFile("Client " + to_string(getIndex()) + " starting task " + to_string(currentTaskId) +
                  " with array size " + to_string(arraySize));

        // Reset tracking structures for new task
        subtaskResults.clear();
        majorityResults.clear();

        // For a new task, reset the server tracking info for the current task
        for (int i = 0; i < numServers; i++) {
            serverTracking[i].score = 0;
            serverTracking[i].subtaskCount = 0;
        }

        // Dispatch subtasks to servers
        dispatchSubtasks();
    }

    void generateDataArray() {
        dataArray.clear();
        for (int i = 0; i < arraySize; i++) {
            dataArray.push_back(intuniform(1, 100));
        }

        stringstream ss;
        ss << "Client " << getIndex() << " generated array: ";
        for (int i = 0; i < min(10, (int)dataArray.size()); i++) {
            ss << dataArray[i] << " ";
        }
        if (dataArray.size() > 10) {
            ss << "... (total " << dataArray.size() << " elements)";
        }
        logToFile(ss.str());
    }

    void divideIntoSubtasks() {
        subtasks.clear();
        int elementsPerSubtask = arraySize / numSubtasks;

        // Ensure each subtask has at least 2 elements as per requirement
        if (elementsPerSubtask < 2) {
            elementsPerSubtask = 2;
            numSubtasks = arraySize / elementsPerSubtask;
            if (numSubtasks == 0) numSubtasks = 1;
        }

        for (int i = 0; i < numSubtasks; i++) {
            vector<int> subtask;
            int start = i * elementsPerSubtask;
            int end = (i == numSubtasks - 1) ? arraySize : (i + 1) * elementsPerSubtask;

            for (int j = start; j < end; j++) {
                subtask.push_back(dataArray[j]);
            }

            subtasks.push_back(subtask);
        }

        // Log subtask division
        for (int i = 0; i < (int)subtasks.size(); i++) {
            stringstream ss;
            ss << "Client " << getIndex() << " subtask " << i << ": ";
            for (int j = 0; j < min(5, (int)subtasks[i].size()); j++) {
                ss << subtasks[i][j] << " ";
            }
            if (subtasks[i].size() > 5) {
                ss << "... (total " << subtasks[i].size() << " elements)";
            }
            logToFile(ss.str());
        }
    }

    void dispatchSubtasks() {
        // Choose servers for each subtask
        int serversPerSubtask = (int)ceil(numServers / 2) + 1;

        for (int subtaskId = 0; subtaskId < (int)subtasks.size(); subtaskId++) {
            vector<int> selectedServers;

            // If this is the second task, select servers based on scores
            if (tasksCompleted > 0) {
                // Sort servers by their average scores
                vector<pair<double, int>> serverRankings;
                for (int i = 0; i < numServers; i++) {
                    serverRankings.push_back({serverAvgScores[i], i});
                }
                sort(serverRankings.rbegin(), serverRankings.rend()); // Sort in descending order

                // Select top servers
                for (int i = 0; i < serversPerSubtask && i < serverRankings.size(); i++) {
                    selectedServers.push_back(serverRankings[i].second);
                }

                // Log server selection strategy
                logToFile("Client " + to_string(getIndex()) + " selecting servers based on scores for task " + to_string(currentTaskId));
            } else {
                // For first task, randomly select servers
                vector<int> allServers;
                for (int i = 0; i < numServers; i++) {
                    allServers.push_back(i);
                }

                // Random shuffle
                shuffle(allServers.begin(), allServers.end(), rng);

                // Pick first serversPerSubtask servers
                for (int i = 0; i < serversPerSubtask && i < allServers.size(); i++) {
                    selectedServers.push_back(allServers[i]);
                }

                // Log server selection strategy
                logToFile("Client " + to_string(getIndex()) + " randomly selecting servers for task " + to_string(currentTaskId));
            }

            // Send subtask to selected servers
            for (int serverId : selectedServers) {
                // Increment subtask count for this server
                serverTracking[serverId].subtaskCount++;

                sendSubtask(subtaskId, subtasks[subtaskId], serverId);
            }

            // Log server selection
            stringstream ss;
            ss << "Client " << getIndex() << " sent subtask " << subtaskId << " to servers: ";
            for (int serverId : selectedServers) {
                ss << serverId << " ";
            }
            logToFile(ss.str());
        }
    }

    void sendSubtask(int subtaskId, const vector<int> &data, int serverId) {
        // Convert data to string
        stringstream ss;
        for (int val : data) {
            ss << val << " ";
        }

        // Create task message
        cMessage *msg = new cMessage("TaskMessage");

        // Add taskId parameter
        msg->addPar("taskId");
        msg->par("taskId") = currentTaskId;

        msg->addPar("subtaskId");
        msg->par("subtaskId") = subtaskId;

        msg->addPar("data");
        msg->par("data") = ss.str().c_str();

        // Send to appropriate server
        send(msg, "out", serverId);
    }

    void handleResultMessage(cMessage *msg) {
        int taskId = msg->par("taskId").longValue();
        int subtaskId = msg->par("subtaskId").longValue();
        int result = msg->par("result").longValue();
        int serverId = msg->par("serverId").longValue();

        // Verify this result belongs to our current task
        if (taskId != currentTaskId) {
            delete msg;
            return; // Ignore results from previous tasks
        }

        // Log received result
        string resultMsg = "Client " + to_string(getIndex()) + " received result: " +
                          to_string(result) + " for subtask " + to_string(subtaskId) +
                          " from server " + to_string(serverId) + " (task " + to_string(taskId) + ")";
        logToFile(resultMsg);

        // Store result
        subtaskResults[subtaskId].push_back({serverId, result});

        // Check if we have received all results for this subtask
        if (subtaskResults[subtaskId].size() >= numServers / 2 + 1) {
            // Determine majority result
            processMajorityResult(subtaskId);

            // Check if all subtasks have been completed
            if (majorityResults.size() == subtasks.size()) {
                // All subtasks completed, compute final result
                computeFinalResult();

                // Broadcast server scores via gossip
                broadcastScores();

                // Schedule next task or end simulation
                if (tasksCompleted < 2) {  // We need to run two tasks as per the assignment
                    // Log completion of the current task
                    logToFile("Client " + to_string(getIndex()) + " completed task " + to_string(currentTaskId) +
                                " and will start the next task soon");

                    // Schedule another task
                    scheduleAt(simTime() + 2.0, new cMessage("StartTask"));
                } else {
                    // Log simulation end
                    logToFile("Client " + to_string(getIndex()) + " has completed all tasks");
                }
            }
        }

        delete msg;
    }

    void processMajorityResult(int subtaskId) {
        // Count occurrences of each result
        unordered_map<int, int> resultCount;
        unordered_map<int, vector<int>> resultToServers;

        for (auto &p : subtaskResults[subtaskId]) {
            int serverId = p.first;
            int result = p.second;

            resultCount[result]++;
            resultToServers[result].push_back(serverId);
        }

        // Find result with majority
        int majorityResult = -1;
        int maxCount = 0;

        for (auto &p : resultCount) {
            if (p.second > maxCount) {
                maxCount = p.second;
                majorityResult = p.first;
            }
        }

        // Store majority result
        majorityResults[subtaskId] = majorityResult;

        // Update server scores
        for (auto &p : subtaskResults[subtaskId]) {
            int serverId = p.first;
            int result = p.second;

            // If server provided correct (majority) result, increment its score
            if (result == majorityResult) {
                serverTracking[serverId].score++;
            }
        }

        // Log majority result
        string majorityMsg = "Client " + to_string(getIndex()) + " determined majority result: " +
                            to_string(majorityResult) + " for subtask " + to_string(subtaskId) +
                            " in task " + to_string(currentTaskId);
        logToFile(majorityMsg);

        // Log honest and malicious servers
        stringstream honestSs, maliciousSs;
        honestSs << "Honest servers for subtask " << subtaskId << " in task " << currentTaskId << ": ";
        maliciousSs << "Malicious servers for subtask " << subtaskId << " in task " << currentTaskId << ": ";

        for (auto &p : subtaskResults[subtaskId]) {
            int serverId = p.first;
            int result = p.second;

            if (result == majorityResult) {
                honestSs << serverId << " ";
            } else {
                maliciousSs << serverId << " ";
            }
        }

        logToFile(honestSs.str());
        logToFile(maliciousSs.str());
    }

    void computeFinalResult() {
        // For finding max element, take max of all subtask results
        finalResult = INT_MIN;
        for (auto &p : majorityResults) {
            finalResult = max(finalResult, p.second);
        }

        // Log final result
        string finalMsg = "Client " + to_string(getIndex()) + " computed final result: " +
                         to_string(finalResult) + " for task " + to_string(currentTaskId);
        logToFile(finalMsg);

        // Log server tracking information
        for (int i = 0; i < numServers; i++) {
            stringstream ss;
            ss << "Client " << getIndex() << " server " << i << " tracking for task " << currentTaskId
               << ": Score=" << serverTracking[i].score
               << ", SubtaskCount=" << serverTracking[i].subtaskCount;

            // Calculate rate if subtasks were assigned
            if (serverTracking[i].subtaskCount > 0) {
                double rate = (double)serverTracking[i].score / serverTracking[i].subtaskCount;
                ss << ", Rate=" << fixed << setprecision(2) << rate;
            } else {
                ss << ", Rate=N/A (no subtasks assigned)";
            }

            logToFile(ss.str());
        }

        // Increment completed tasks
        tasksCompleted++;
    }

    void broadcastScores() {
        // Create score message with the server tracking information
        string scoreStr = to_string(getIndex()) + ":";

        for (int i = 0; i < numServers; i++) {
            // Format: serverId=score:subtaskCount
            scoreStr += to_string(i) + "=" + to_string(serverTracking[i].score) +
                       ":" + to_string(serverTracking[i].subtaskCount);

            if (i < numServers - 1) {
                scoreStr += ",";
            }
        }

        // Create gossip message
        cMessage *gossip = new cMessage("GossipMessage");
        gossip->addPar("timestamp");
        gossip->par("timestamp") = simTime().dbl();

        gossip->addPar("score");
        gossip->par("score") = scoreStr.c_str();

        gossip->addPar("taskNumber");
        gossip->par("taskNumber") = currentTaskId;

        // Format for message log
        string msgKey = to_string(simTime().dbl()) + ":" + to_string(getIndex()) + ":" + scoreStr;
        messageLog[msgKey] = true;

        // Log gossip message
        string gossipMsg = "Client " + to_string(getIndex()) + " broadcasting scores for task " +
                          to_string(currentTaskId) + ": " + scoreStr;
        logToFile(gossipMsg);

        // Send to all connected clients
        for (int i = 0; i < gateSize("gout"); i++) {
            cMessage *copy = gossip->dup();
            send(copy, "gout", i);
        }

        delete gossip;
    }

    void handleGossipMessage(cMessage *msg) {
        double timestamp = msg->par("timestamp").doubleValue();
        string scoreStr = msg->par("score").stringValue();
        int taskNumber = 1;  // Default to task 1

        // Check if taskNumber parameter exists
        if (msg->hasPar("taskNumber")) {
            taskNumber = msg->par("taskNumber").longValue();
        }

        // Create message key
        string msgKey = to_string(timestamp) + ":" + scoreStr;

        // Check if this message has been seen before
        if (messageLog.find(msgKey) != messageLog.end()) {
            // Already seen this message, ignore
            delete msg;
            return;
        }

        // Mark message as seen
        messageLog[msgKey] = true;

        // Log received gossip
        string gossipLog = "Client " + to_string(getIndex()) + " received gossip for task " +
                          to_string(taskNumber) + ": " + to_string(timestamp) + ":" + scoreStr +
                          " from gate " + to_string(msg->getArrivalGate()->getIndex());
        logToFile(gossipLog);

        // Forward to other clients
        for (int i = 0; i < gateSize("gout"); i++) {
            if (i != msg->getArrivalGate()->getIndex()) {
                cMessage *copy = msg->dup();
                send(copy, "gout", i);
            }
        }

        // Process the scores
        processReceivedScores(scoreStr, taskNumber);

        delete msg;
    }

    void processReceivedScores(const string &scoreStr, int taskNumber) {
        // Parse score string - format: clientId:serverId1=score1:subtaskCount1,serverId2=score2:subtaskCount2,...
        size_t colonPos = scoreStr.find(':');
        if (colonPos == string::npos) return;

        int clientId = stoi(scoreStr.substr(0, colonPos));
        string scoresSection = scoreStr.substr(colonPos + 1);

        // Parse individual server scores
        istringstream ss(scoresSection);
        string token;

        while (getline(ss, token, ',')) {
            size_t equalsPos = token.find('=');
            if (equalsPos == string::npos) continue;

            size_t colonPos = token.find(':', equalsPos);
            if (colonPos == string::npos) continue;

            int serverId = stoi(token.substr(0, equalsPos));
            int score = stoi(token.substr(equalsPos + 1, colonPos - equalsPos - 1));
            int subtaskCount = stoi(token.substr(colonPos + 1));

            // Add to aggregated tracking
            aggregatedServerTracking[serverId].score += score;
            aggregatedServerTracking[serverId].subtaskCount += subtaskCount;

            // Recalculate average score using the new formula
            if (aggregatedServerTracking[serverId].subtaskCount > 0) {
                serverAvgScores[serverId] = (double)aggregatedServerTracking[serverId].score /
                                        aggregatedServerTracking[serverId].subtaskCount;
            } else {
                serverAvgScores[serverId] = 0.0;
            }
        }

        // Log updated average scores
        stringstream avgSs;
        avgSs << "Client " << getIndex() << " updated average scores after task " << taskNumber << ": ";
        for (int i = 0; i < numServers; i++) {
            avgSs << i << ":" << fixed << setprecision(2) << serverAvgScores[i]
                  << " (Score=" << aggregatedServerTracking[i].score
                  << ", Tasks=" << aggregatedServerTracking[i].subtaskCount << ") ";
        }
        logToFile(avgSs.str());
    }

    void logToFile(const string &message) {
        // Get current time
        time_t now = time(nullptr);
        tm *ltm = localtime(&now);

        stringstream timeStr;
        timeStr << setfill('0')
                << setw(2) << ltm->tm_hour << ":"
                << setw(2) << ltm->tm_min << ":"
                << setw(2) << ltm->tm_sec;

        // Print to console
        EV << "[" << timeStr.str() << "] " << message << endl;

        // Write to file
        ofstream outFile(OUTPUT_FILE, ios::app);
        if (outFile.is_open()) {
            outFile << "[" << simTime() << "] [" << timeStr.str() << "] " << message << endl;
            outFile.close();
        }
    }
};

Define_Module(Client);
