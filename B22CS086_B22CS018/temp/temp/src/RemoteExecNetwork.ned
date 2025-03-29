
package temp;

simple Client
{
    parameters:
        int arraySize;
        int numSubtasks;
        int numServers;
        int numClients;
    gates:
        input in[];   // message from server
        output out[]; // sending to server
        input gin[];  // gossip send
        output gout[]; // gossip receive
}

simple Server
{
    gates:
        input in[];   // receiving from client
        output out[]; // sending to client
}

network RemoteExecNetwork
{
    parameters:
        int numClients = default(3);
        int numServers = default(5);
    submodules:
        client[numClients]: Client {
            parameters:
                arraySize = 99;
                numSubtasks = 3;
                numServers = 5;
                numClients = 3;
        }
        server[numServers]: Server;
    connections allowunconnected:
        client[0].out++ --> server[0].in++;
        server[0].out++ --> client[0].in++;
        client[0].out++ --> server[1].in++;
        server[1].out++ --> client[0].in++;
        client[0].out++ --> server[2].in++;
        server[2].out++ --> client[0].in++;
        client[0].out++ --> server[3].in++;
        server[3].out++ --> client[0].in++;
        client[0].out++ --> server[4].in++;
        server[4].out++ --> client[0].in++;
        client[1].out++ --> server[0].in++;
        server[0].out++ --> client[1].in++;
        client[1].out++ --> server[1].in++;
        server[1].out++ --> client[1].in++;
        client[1].out++ --> server[2].in++;
        server[2].out++ --> client[1].in++;
        client[1].out++ --> server[3].in++;
        server[3].out++ --> client[1].in++;
        client[1].out++ --> server[4].in++;
        server[4].out++ --> client[1].in++;
        client[2].out++ --> server[0].in++;
        server[0].out++ --> client[2].in++;
        client[2].out++ --> server[1].in++;
        server[1].out++ --> client[2].in++;
        client[2].out++ --> server[2].in++;
        server[2].out++ --> client[2].in++;
        client[2].out++ --> server[3].in++;
        server[3].out++ --> client[2].in++;
        client[2].out++ --> server[4].in++;
        server[4].out++ --> client[2].in++;
        client[0].gout++ --> client[1].gin++;
        client[0].gout++ --> client[2].gin++;
        client[1].gout++ --> client[0].gin++;
        client[1].gout++ --> client[2].gin++;
        client[2].gout++ --> client[0].gin++;
        client[2].gout++ --> client[1].gin++;
}