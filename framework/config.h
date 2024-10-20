#ifndef CONFIG_H
#define CONFIG_H

#include "dotenv.h"
#include <map>

class Config {
private:
    int totalNodes;
    // int timeout;
    std::map<int, std::pair<std::string, int>> nodeConfigs; // cau hinh cho tung node: id - ip - port

public:
    Config() {
        dotenv::init("config.env");
        loadConfigurations();
    }

    int getTotalNodes() const {
        return totalNodes;
    }

    // int getTimeout() const {
    //     return timeout;
    // }

    std::string getNodeIp(int nodeId) const {
        auto it = nodeConfigs.find(nodeId);
        if (it != nodeConfigs.end()) {
            return it->second.first;
        }
        throw std::runtime_error("Node ID " + std::to_string(nodeId) + " not found");
    }

    int getNodePort(int nodeId) const {
        auto it = nodeConfigs.find(nodeId);
        if (it != nodeConfigs.end()) {
            return it->second.second;
        }
        throw std::runtime_error("Node ID " + std::to_string(nodeId) + " not found");
    }

    std::map<int, std::pair<std::string, int>> getNodeConfigs() { 
        return nodeConfigs;
    }
    
private:
    void loadConfigurations() { // tai file cau hinh len
        try {
            totalNodes = std::stoi(dotenv::getenv("TOTAL_NODES"));
            // timeout = std::stoi(dotenv::getenv("TIMEOUT"));
            if (totalNodes <= 0) {
                throw std::runtime_error("TOTAL_NODES must be greater than 0\n");
            }

            for (int i = 1; i <= totalNodes; i++) {
                std::string ip = dotenv::getenv(("NODE_" + std::to_string(i) + "_IP").c_str());
                int port = std::stoi(dotenv::getenv(("NODE_" + std::to_string(i) + "_PORT").c_str()));
                if (ip.empty() || port <= 0 || port > 65535) {
                    throw std::runtime_error("Invalid address or port for node " + std::to_string(i) + "\n");
                }

                nodeConfigs[i] = std::make_pair(ip, port);
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Configuration error: " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
};

extern Config config;


#endif