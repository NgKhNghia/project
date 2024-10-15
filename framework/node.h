#ifndef NODE_H
#define NODE_H

#include <string>
#include <fstream>
#include <map>
#include <thread>
#include "comm.h"
#include "config.h"

class Node {
public:
    enum class NodeState {ACTIVE, INACTIVE, FAILED};

private:
    int id;
    std::string ip;
    int port;
    NodeState state;
    std::shared_ptr<Config> config;      // dung de cau hinh
    std::shared_ptr<Comm> comm; // dung de giao tiep giua cac node
    std::ofstream logFile;  // dung de ghi file log


public:
    // Node(int id, const std::string &ip, int port);
    Node(int id, const std::string &ip, int port, std::shared_ptr<Comm> comm, std::shared_ptr<Config> config)
        : id(id), ip(ip), port(port), comm(comm), config(config), state(NodeState::INACTIVE) {
        logFile.open("node_" + std::to_string(id) + "_log.txt", std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error opening log file for Node " << id << std::endl;
        }
    }

    ~Node() {
        if (logFile.is_open()) {
            logMessage("Node " + std::to_string(id) + "closed");
            logFile.close();
        }
    }

    void initialize() {
        setState(NodeState::ACTIVE);
        std::cout << "Node " << id << " created with ip " << ip << " port " << port << "\n";
        logMessage("Connection initialized");
    }

    int getId() const { 
        return id; 
    }

    std::string getIp() const { 
        return ip; 
    }

    int getPort() const { 
        return port; 
    }

    Node::NodeState getState() const { 
        return state; 
    }

    void setState(NodeState state) {
        this->state = state;
    }

    std::map<int, std::pair<std::string, int>> getNodeConfigs() { 
        return config->getNodeConfigs();
    }

    void sendMessage(int receiverId, const std::string &message) { // khong co tre
        try {
            comm->send(receiverId, message);
            logMessage("[Sent message to Node " + std::to_string(receiverId) + "] " + message);
        } 
        catch (const std::exception &e) {
            logMessage("Failed to send message: " + std::string(e.what()));
        }
    }

    // void sendDelayedMessage(const Node &receiver, const std::string &message, int delayMilliseconds); // gia lap tre
    
    std::string receiveMessage() {
        try {
            std::string message = comm->getMessage();
            logMessage("[Received message] " + message);
            return message;
        } 
        catch (const std::exception &e) {
            logMessage("Failed to receive message: " + std::string(e.what()));
            return "";
        }
    }

private:
    void logMessage(const std::string &message) {
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::string timeStr = std::ctime(&time);
            timeStr = timeStr.substr(0, timeStr.size() - 1);
            logFile << "[" << timeStr << "]" << " " << message << std::endl;
        } else {
            std::cerr << "Log file not open for Node " << id << std::endl;
        }
    }

};

#endif 
