#include "config.h"
#include <iostream>

Config::Config() {
    dotenv::init("config.env");
    loadConfigurations();
}

int Config::getTotalNodes() const {
    return totalNodes;
}

int Config::getTimeout() const {
    return timeout;
}

std::string Config::getNodeAddress(int nodeId) const {
    auto it = nodeConfigs.find(nodeId);
    if (it != nodeConfigs.end()) {
        return it->second.first;
    }
    throw std::runtime_error("Node ID " + std::to_string(nodeId) + " not found");
}

int Config::getNodePort(int nodeId) const {
    auto it = nodeConfigs.find(nodeId);
    if (it != nodeConfigs.end()) {
        return it->second.second;
    }
    throw std::runtime_error("Node ID " + std::to_string(nodeId) + " not found");
}

std::map<int, std::pair<std::string, int>> Config::getNodeConfigs() {
    return nodeConfigs;
}


void Config::loadConfigurations() {
    try {
        totalNodes = std::stoi(dotenv::getenv("TOTAL_NODES"));
        timeout = std::stoi(dotenv::getenv("TIMEOUT"));
        if (totalNodes <= 0) {
            throw std::runtime_error("TOTAL_NODES must be greater than 0\n");
        }

        for (int i = 1; i <= totalNodes; i++) {
            std::string address = dotenv::getenv(("NODE_" + std::to_string(i) + "_ADDRESS").c_str());
            int port = std::stoi(dotenv::getenv(("NODE_" + std::to_string(i) + "_PORT").c_str()));
            if (address.empty() || port <= 0 || port > 65535) {
                throw std::runtime_error("Invalid address or port for node " + std::to_string(i) + "\n");
            }

            nodeConfigs[i] = std::make_pair(address, port);
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}