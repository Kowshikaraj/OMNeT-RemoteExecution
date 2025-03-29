#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <map>
#include <set>
#include <random>
#include <vector>

class MasterServer {
private:
    // Singleton instance
    static MasterServer* instance;

    // Map from (clientId, taskId) pair to set of malicious server IDs
    std::map<std::pair<int, int>, std::set<int>> maliciousServers;

    // Total number of servers in the system
    int totalServers;

    // Random number generator
    std::mt19937 rng;

    // Private constructor for singleton
    MasterServer() : totalServers(0) {
        // Seed the random number generator
        std::random_device rd;
        rng.seed(rd());
    }

public:
    // Get singleton instance
    static MasterServer* getInstance() {
        if (instance == nullptr) {
            instance = new MasterServer();
        }
        return instance;
    }

    // Set the total number of servers
    void setTotalServers(int count) {
        totalServers = count;
    }

    // Determine which servers are malicious for a given client task
    void determineServerBehavior(int clientId, int taskId) {
        std::pair<int, int> key = std::make_pair(clientId, taskId);

        // If already determined, don't recompute
        if (maliciousServers.find(key) != maliciousServers.end()) {
            return;
        }

        // Maximum malicious servers allowed: strictly less than n/4
        int maxMalicious = totalServers / 4;

        // Select a random number of servers to be malicious (0 to maxMalicious-1)
        std::uniform_int_distribution<> numDist(0,maxMalicious - 1);
        int numMalicious = numDist(rng);

        // Create a vector of all server IDs
        std::vector<int> allServers;
        for (int i = 0; i < totalServers; i++) {
            allServers.push_back(i);
        }

        // Shuffle the servers
        std::shuffle(allServers.begin(), allServers.end(), rng);

        // Select the first numMalicious servers to be malicious
        std::set<int> malicious;
        for (int i = 0; i < numMalicious; i++) {
            malicious.insert(allServers[i]);
        }

        // Store the malicious servers for this client task
        maliciousServers[key] = malicious;
    }

    // Check if a specific server is malicious for a given client task
    bool isServerMalicious(int clientId, int taskId, int serverId) {
        std::pair<int, int> key = std::make_pair(clientId, taskId);

        // Determine server behavior if not already done
        if (maliciousServers.find(key) == maliciousServers.end()) {
            determineServerBehavior(clientId, taskId);
        }

        // Check if this server is in the malicious set
        return maliciousServers[key].find(serverId) != maliciousServers[key].end();
    }

    // For debugging: get the total count of malicious servers for a client task
    int getMaliciousCount(int clientId, int taskId) {
        std::pair<int, int> key = std::make_pair(clientId, taskId);

        if (maliciousServers.find(key) == maliciousServers.end()) {
            return 0;
        }

        return maliciousServers[key].size();
    }
};

// Initialize the static instance
MasterServer* MasterServer::instance = nullptr;

#endif // MASTERSERVER_H
