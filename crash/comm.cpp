#include "comm.h"
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

Comm::Comm(int port, std::shared_ptr<Config> config) : config(config) {
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Creating socket failed\n";
        throw std::runtime_error("Creating socket failed");
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        close(serverSocket);
        std::cerr << "Bind failed\n";
        throw std::runtime_error("Bind failed");
    }

    if (::listen(serverSocket, 5) < 0) {
        std::cerr << "Listen failed\n";
        throw std::runtime_error("Listen failed");
        close(serverSocket);
    }

    // listenThread = std::thread(&Comm::acceptClients, this);
    receiveThread = std::thread(&Comm::receive, this);
}

Comm::~Comm() {
    if (listenThread.joinable()) {
        listenThread.join();
    }
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
    close(serverSocket);
}

void Comm::send(int destId, const std::string &message) {
    auto it = config->getNodeConfigs().find(destId);
    if (it == config->getNodeConfigs().end()) {
        std::cout << "Destination ID " << destId << " not found";
        throw std::runtime_error("Destination ID " + std::to_string(destId) + " not found");
        return;
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create client socket\n";
        throw std::runtime_error("Failed to create client socket");
        return;
    }

    sockaddr_in destIp;
    destIp.sin_family = AF_INET;
    destIp.sin_port = htons(it->second.second);
    inet_pton(AF_INET, it->second.first.c_str(), &destIp.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&destIp, sizeof(destIp)) < 0) {
        throw std::runtime_error("Failed to connect to destination");
        std::cerr << "Failed to connect to destination\n";
        close(clientSocket);
        return;
    }

    if (::send(clientSocket, message.c_str(), message.size(), 0) < 0) {
        throw std::runtime_error("Failed to send message");
        std::cerr << "Failed to send message\n";
    }

    close(clientSocket);
}

void Comm::receive() {  
    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket >= 0) {
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN); // lay ip 
            std::string clientIp(clientIP);
            int clientPort = ntohs(clientAddr.sin_port); // lay port
            int clientId;
            for (auto it : config->getNodeConfigs()) { // lay id
                if (it.second.first == clientIp) {
                    clientId = it.first;
                    break;
                }
            }

            char buffer[1024] = {0};
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead < 0) {
                std::cerr << "Failed to receive message\n";
                throw std::runtime_error("Failed to receive message");
            }
            else {
                buffer[bytesRead] = '\0';
                {
                    std::lock_guard<std::mutex> lock(socketMutex);
                    messageQueue.emplace(clientId, std::string(buffer));
                    messageAvailable.notify_one();
                }
            }
            close(clientSocket);
        }
    }
}

std::pair<int, std::string> Comm::getMessage() {
    std::unique_lock<std::mutex> lock(socketMutex);
    messageAvailable.wait(lock, [this] { return !messageQueue.empty(); });

    auto message = messageQueue.front();
    messageQueue.pop();

    return message;
}

// void Comm::broadcast(const std::string &message, const std::vector<int>& destinationIds) {
//     for (int id : destinationIds) {
//         send(message, id);
//     }
// }

// void Comm::multicast(const std::string &message, const std::vector<int>& groupIds) {
//     for (int id : groupIds) {
//         send(message, id);
//     }
// }

// void Comm::setDelay(int messageId, int delayMilliseconds) {
//     messageDelays[messageId] = delayMilliseconds;
// }
