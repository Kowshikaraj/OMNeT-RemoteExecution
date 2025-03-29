#include <omnetpp.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <map>
#include "MasterServer.h"

using namespace omnetpp;
using namespace std;

const string OUTPUT = "output.txt";

class Server : public cSimpleModule
{
protected:
    // Reference to the MasterServer
    MasterServer* masterServer;

    void initialize() override {
        // Get the MasterServer instance
        masterServer = MasterServer::getInstance();

        // Set the total number of servers (only needed once, but safe to call multiple times)
        int numServers = getParentModule()->par("numServers");
        masterServer->setTotalServers(numServers);
    }

    void handleMessage(cMessage *msg) override {
        if (strcmp(msg->getName(), "TaskMessage") == 0) {
            int taskId = msg->par("taskId").longValue();
            int subtaskId = msg->par("subtaskId").longValue();
            string dataStr = msg->par("data").stringValue();

            // Get clientId from the arrival gate
            int clientId = msg->getArrivalGate()->getIndex();

            // Check with MasterServer if this server should be malicious
            bool isHonest = !masterServer->isServerMalicious(clientId, taskId, getIndex());

            istringstream iss(dataStr);
            vector<int> data;

            int value;
            while (iss >> value) {
                data.push_back(value);
            }

            // Log received task
            {
                ofstream out(OUTPUT, ios::app);
                if (!out.is_open()) {
                    cout << "Error opening file " << OUTPUT << "\n";
                    return;
                } else {
                    out << convertMsgToString(msg);
                    out.close();
                }
            }

            // Compute the maximum
            int maxi = *max_element(data.begin(), data.end());

            // If malicious for this task, modify the result
            if (!isHonest) {
                maxi -= intuniform(1, 10); // Sabotage the result
            }

            // Create and send a ResultMessage back
            cMessage *rm = new cMessage("ResultMessage");

            rm->addPar("taskId");
            rm->par("taskId") = taskId;

            rm->addPar("subtaskId");
            rm->par("subtaskId") = subtaskId;

            rm->addPar("result");
            rm->par("result") = maxi;

            rm->addPar("serverId");
            rm->par("serverId") = getIndex();

            string temp = "Result Server:" + to_string(getIndex()) +
                " taskId:" + to_string(taskId) +
                " subtaskId:" + to_string(subtaskId) +
                " on gate:" + to_string(msg->getArrivalGate()->getIndex()) +
                " result:" + to_string(maxi) +
                " isHonest:" + (isHonest ? "true" : "false") + "\n";

            // Log sent result
            {
                ofstream out(OUTPUT, ios::app);
                if (!out.is_open()) {
                    cout << "Error opening file " << OUTPUT << "\n";
                    return;
                } else {
                    out << temp;
                    out.close();
                }
            }

            // Send back to the client that sent the request
            send(rm, "out", msg->getArrivalGate()->getIndex());

            delete msg;
        } else {
            delete msg;
        }
    }

    string convertMsgToString(cMessage *msg) {
        string str = "";
        if(strcmp(msg->getName(), "TaskMessage") == 0) {
            str += "TaskMessage: ";
            str += "Server: " + to_string(getIndex()) + " on gate: " + to_string(msg->getArrivalGate()->getIndex()) + " ";
            str += "taskId: " + to_string(msg->par("taskId").longValue()) + " ";
            str += "subtaskId: " + to_string(msg->par("subtaskId").longValue()) + " ";
            str += "data: " + string(msg->par("data").stringValue()) + "\n";
        }
        return str;
    }
};

Define_Module(Server);
