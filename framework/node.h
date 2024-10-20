#ifndef NODE_H
#define NODE_H

#include <string>
#include <fstream>
#include <map>
#include <thread>
#include "comm.h"
#include "config.h"
#include "log.h"

extern Config config;
extern Logger logger;


class Node {
public:
    // enum class NodeState {ACTIVE, INACTIVE, FAILED};
    
protected:
    int id;
    std::string ip;
    int port;
    // NodeState state;
    std::shared_ptr<Comm> comm; // dung de giao tiep giua cac node


public:
    Node(int id, const std::string &ip, int port, std::shared_ptr<Comm> comm)
        : id(id), ip(ip), port(port), comm(comm) {
    }
  
    ~Node() {
        logger.log("Node " + std::to_string(id) + " closed");
    }

    void initialize() {
        // setState(NodeState::ACTIVE);
        logger.log("Node " + std::to_string(id) + " created");
    }

    // int getId() const { 
    //     return id; 
    // }

    // std::string getIp() const { 
    //     return ip; 
    // }

    // int getPort() const { 
    //     return port; 
    // }

    // Node::NodeState getState() const { 
    //     return state; 
    // }

    // void setState(NodeState state) {
    //     this->state = state;
    // }

    


};

#endif 
