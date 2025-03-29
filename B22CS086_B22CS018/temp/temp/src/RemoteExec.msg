message TaskMessage {
    int taskId;       // To identify which task this subtask belongs to
    int subtaskId;    // The ID of the subtask within the task
    string data;      // The data for the subtask
}

message ResultMessage {
    int taskId;       // The task ID this result belongs to
    int subtaskId;    // The subtask ID within the task
    int result;       // The computed result
    int serverId;     // The ID of the server that processed this
}

message GossipMessage {
    double timestamp;
    string score;
    int taskNumber;
}