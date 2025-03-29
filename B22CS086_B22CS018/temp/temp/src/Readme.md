# Remote Execution Simulation in OMNeT++

## Overview
This project implements a remote program execution simulation using OMNeT++ Discrete Event Simulator. The system consists of client nodes and server nodes where each client divides a task into n subtasks and sends them to n/2 + 1 servers. The client then determines the correct result based on the majority outcome.

## Features
- Dynamic network generation with configurable number of clients and servers
- Server nodes that can behave honestly or maliciously (maximum n/4 can be malicious)
- Task division and distribution by client nodes
- Majority-based result verification
- Server rating system based on honest/malicious behavior
- Gossip protocol for distributing server ratings between clients
- Adaptive server selection based on accumulated ratings

## Files
- `generate_ned.py`: Python script to dynamically generate the .ned file and topology file
- `RemoteExecNetwork.ned`: Network description file (generated)
- `topo.txt`: Topology configuration file (generated)
- `client.cc`: Implementation of client node behavior
- `server.cc`: Implementation of server node behavior

## Requirements
- OMNeT++ (>= 5.0)
- Python 3.x

## Installation
1. Install OMNeT++ according to the instructions in the [OMNeT++ Installation Guide](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf)
2. Clone this repository or extract the project files
3. Run the Python script to generate the network description file and topology file

## Generating Network Files
You can use the provided Python script to generate the .ned file and topology file:

```bash
python generate_ned.py --clients 3 --servers 5 --array-size 100
```

Parameters:
- `--clients`: Number of client nodes (default: 3)
- `--servers`: Number of server nodes (default: 5)
- `--array-size`: Size of the array for processing (default: 100)
- `--subtasks`: Number of subtasks (default: equals number of servers)
- `--output`: Output .ned file name (default: RemoteExecNetwork.ned)
- `--topo-file`: Output topology file name (default: topo.txt)

## Compilation
```bash
cd /path/to/project
opp_makemake
make
```

## Running the Simulation
```bash
./remoteexec -f omnetpp.ini
```

## Configuration
The simulation uses the topology file `topo.txt` for configuration. You can edit this file to modify:
- Number of clients and servers
- Which servers can behave maliciously
- Network connections between nodes

## Simulation Flow
1. Network initialization according to topology file
2. Clients generate tasks (arrays of integers)
3. Tasks are divided into n subtasks
4. Each subtask is sent to n/2 + 1 servers
5. Servers process subtasks (honestly or maliciously)
6. Clients collect results and determine the correct outcome using majority rule
7. Clients rate servers based on their behavior
8. Clients share server ratings using the gossip protocol
9. For the second round, clients select servers based on accumulated ratings

## Output
The simulation produces the following outputs:
- Console logs showing subtask results from servers
- Console logs showing subtask results received by clients
- Consolidated task results from clients
- Gossip messages with timestamps and source IPs
- Server scores displayed by clients

## Notes
- The maximum number of malicious servers is limited to n/4 where n is the total number of servers
- Each subtask contains at least 2 elements from the original array
- The task in this simulation is finding the maximum element in an array

## Troubleshooting
- If you encounter connection issues, check the topology file and ensure all connections are properly defined
- For compilation errors, ensure you have the correct version of OMNeT++ installed
- If servers behave unexpectedly, check the server behavior settings in the code