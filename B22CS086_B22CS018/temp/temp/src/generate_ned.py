import sys
import os

def generate_ned_file(output_filename, num_clients, num_servers, array_size, num_subtasks):
    """
    Generate a .ned file for the RemoteExecNetwork with the specified parameters.
    
    Args:
        output_filename (str): Name of the output .ned file
        num_clients (int): Number of client nodes
        num_servers (int): Number of server nodes
        array_size (int): Size of the array for tasks
        num_subtasks (int): Number of subtasks to divide the task into
    """
    # Open the file for writing
    with open(output_filename, 'w') as f:
        # Write package and imports
        f.write("package temp;\n\n")
        
        # Write client module definition
        f.write("simple Client\n{\n")
        f.write("parameters:\n")
        f.write("    int arraySize;\n")
        f.write("    int numSubtasks;\n")
        f.write("    int numServers;\n")
        f.write("    int numClients;\n")
        f.write("gates:\n")
        f.write("    input in[]; // message from server\n")
        f.write("    output out[]; // sending to server\n")
        f.write("    input gin[]; // gossip receive\n")
        f.write("    output gout[]; // gossip send\n")
        f.write("}\n\n")
        
        # Write server module definition
        f.write("simple Server\n{\n")
        f.write("gates:\n")
        f.write("    input in[]; // receiving from client\n")
        f.write("    output out[]; // sending to client\n")
        f.write("}\n\n")
        
        # Start RemoteExecNetwork definition
        f.write("network RemoteExecNetwork\n{\n")
        f.write("parameters:\n")
        f.write(f"    int numClients = default({num_clients});\n")
        f.write(f"    int numServers = default({num_servers});\n")
        
        # Define submodules
        f.write("submodules:\n")
        f.write("    client[numClients]: Client {\n")
        f.write("        parameters:\n")
        f.write(f"            arraySize = {array_size};\n")
        f.write(f"            numSubtasks = {num_subtasks};\n")
        f.write(f"            numServers = {num_servers};\n")
        f.write(f"            numClients = {num_clients};\n")
        f.write("    }\n")
        f.write("    server[numServers]: Server;\n")
        
        # Start connections section
        f.write("connections allowunconnected:\n")
        
        # Client-server connections
        for client_id in range(num_clients):
            for server_id in range(num_servers):
                # Client → Server
                f.write(f"    client[{client_id}].out++ --> server[{server_id}].in++;\n")
                # Server → Client
                f.write(f"    server[{server_id}].out++ --> client[{client_id}].in++;\n")
        
        # Client-client connections for gossip protocol
        for src_client in range(num_clients):
            for dst_client in range(num_clients):
                if src_client != dst_client:  # Don't connect a client to itself
                    f.write(f"    client[{src_client}].gout++ --> client[{dst_client}].gin++;\n")
        
        # End network definition
        f.write("}\n")
    
    print(f"NED file generated: {output_filename}")

def read_topology_file(topo_file):
    """
    Read the topology from a configuration file.
    Expected format:
    num_clients=<num>
    num_servers=<num>
    array_size=<num>
    num_subtasks=<num>
    """
    config = {
        'num_clients': 3,  # Default values
        'num_servers': 5,
        'array_size': 99,
        'num_subtasks': 3
    }
    
    try:
        with open(topo_file, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue  # Skip empty lines and comments
                    
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()
                
                if key == 'num_clients':
                    config['num_clients'] = int(value)
                elif key == 'num_servers':
                    config['num_servers'] = int(value)
                elif key == 'array_size':
                    config['array_size'] = int(value)
                elif key == 'num_subtasks':
                    config['num_subtasks'] = int(value)
    except FileNotFoundError:
        print(f"Warning: Topology file {topo_file} not found. Using default values.")
    except Exception as e:
        print(f"Error reading topology file: {e}")
        print("Using default values.")
        
    return config

def main():
    """
    Main function to parse command line arguments and generate the NED file.
    """
    # Default values
    output_file = "RemoteExecNetwork.ned"
    topo_file = "topo.txt"
    
    # Parse command line arguments
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "-o" and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "-t" and i + 1 < len(sys.argv):
            topo_file = sys.argv[i + 1]
            i += 2
        else:
            i += 1
    
    # Read topology from file
    config = read_topology_file(topo_file)
    
    # Generate NED file
    generate_ned_file(
        output_file,
        config['num_clients'],
        config['num_servers'],
        config['array_size'],
        config['num_subtasks']
    )
    
    print("\nYou can modify the topology by editing the file:", topo_file)
    print("Format example:")
    print("num_clients=3")
    print("num_servers=5")
    print("array_size=99")
    print("num_subtasks=3")

if _name_ == "_main_":
    # If topo.txt doesn't exist, create it with default values
    if not os.path.exists("topo.txt"):
        with open("topo.txt", "w") as f:
            f.write("# Topology configuration\n")
            f.write("num_clients=3\n")
            f.write("num_servers=5\n")
            f.write("array_size=99\n")
            f.write("num_subtasks=3\n")
        print("Created default topo.txt file")
        
    main()