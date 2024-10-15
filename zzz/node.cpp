#include "node.h"
#include <iostream>

// Node::Node(int id, const std::string &address, int port) : id(id), address(address), port(port) {}

Node::Node(int id, const std::string &address, int port, std::shared_ptr<Comm> comm, std::shared_ptr<Config> config)
    : id(id), address(address), port(port), comm(comm), config(config), state(NodeState::INACTIVE) {
    logFile.open("node_" + std::to_string(id) + "_log.txt", std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Error opening log file for Node " << id << std::endl;
    }
}

Node::~Node() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

int Node::getId() const { 
    return id; 
}

std::string Node::getAddress() const { 
    return address; 
}

int Node::getPort() const { 
    return port; 
}

Node::NodeState Node::getState() const { 
    return state; 
}

void Node::setState(NodeState state) {
    this->state = state;
}

void Node::initialize() {
    setState(NodeState::ACTIVE);
    std::cout << "Node " << id << "created with address " << address << " port " << port << "\n";
    logMessage("Connection initialized\n");
}

void Node::sendMessage(int receiverId, const std::string &message) {
    try {
        comm->send(receiverId, message);
        logMessage("Sent message to Node " + std::to_string(receiverId) + ": " + message);
    } 
    catch (const std::exception &e) {
        logMessage("Failed to send message: " + std::string(e.what()));
    }
}

// void Node::sendDelayedMessage(const Node &receiver, const std::string &message, int delayMilliseconds) {
//     // communication->setDelay(id, delayMilliseconds);
//     std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));
//     sendMessage(receiver, message);
// }

std::pair<int, std::string> Node::receiveMessage() {
    try {
        std::pair<int, std::string> message = comm->getMessage();
        logMessage("Received message from " + std::to_string(message.first) + ": " + message.second);
        return message;
    } 
    catch (const std::exception &e) {
        logMessage("Failed to receive message: " + std::string(e.what()));
        return std::make_pair(-1, "");
    }
}

void Node::logMessage(const std::string &message) {
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        logFile << std::ctime(&time) << " [Node " << id << "]: " << message << std::endl;
    } else {
        std::cerr << "Log file not open for Node " << id << std::endl;
    }
}