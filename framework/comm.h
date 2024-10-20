#ifndef COMM_H
#define COMM_H

#include "config.h"
#include "log.h"
#include <string>
#include <cstring>
#include <queue>
#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <condition_variable>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

extern Config config;
extern Logger logger;

class Comm {
private:
    int serverSocket;
    int opt = 1;
    struct sockaddr_in servaddr;
    std::mutex socketMutex;                      
    std::condition_variable messageAvailable;
    std::queue<std::string> messageQueue; 
    std::thread receiveThread;

public:
    Comm(int port) {
        if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            // std::cerr << "Creating socket failed\n";
            throw std::runtime_error("Creating socket failed");
        }

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);

        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            std::cout << "Loi gan lai socket option!\n";
            exit(1);
        }

        if (bind(serverSocket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            close(serverSocket);
            // std::cerr << "Bind failed\n";
            throw std::runtime_error("Bind failed");
        }

        if (::listen(serverSocket, 5) < 0) {
            // std::cerr << "Listen failed\n";
            throw std::runtime_error("Listen failed");
            close(serverSocket);
        }

        receiveThread = std::thread(&Comm::receive, this);
    }

    ~Comm() {
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        close(serverSocket);
    }

    void send(int destId, const std::string &message) {
        auto it = config.getNodeConfigs().find(destId);

        if (it == config.getNodeConfigs().end()) {
            std::cout << "Destination ID " << destId << " not found";
            // return;
            throw std::runtime_error("Destination ID " + std::to_string(destId) + " not found");
        }

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            close(clientSocket);
            // std::cerr << "Failed to create client socket\n";
            throw std::runtime_error("Failed to create client socket");
        }

        sockaddr_in destIp;
        destIp.sin_family = AF_INET;
        destIp.sin_port = htons(it->second.second);
        inet_pton(AF_INET, it->second.first.c_str(), &destIp.sin_addr);

        if (connect(clientSocket, (struct sockaddr*)&destIp, sizeof(destIp)) < 0) {
            close(clientSocket);
            // std::cerr << "Failed to connect to destination\n";
            throw std::runtime_error("Failed to connect to destination");
        }

        if (::send(clientSocket, message.c_str(), message.size(), 0) < 0) {
            close(clientSocket);
            // std::cerr << "Failed to send message\n";
            throw std::runtime_error("Failed to send message");
        }

        close(clientSocket);
    }

    void receive() {  
        while (1) {
            int clientSocket = accept(serverSocket, nullptr, nullptr);

            if (clientSocket >= 0) {
                char buffer[1024] = {0};
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead < 0) {
                    close(clientSocket);
                    // std::cerr << "Failed to receive message\n";
                    throw std::runtime_error("Failed to receive message");
                }
                else {
                    buffer[bytesRead] = '\0';
                    {
                        std::lock_guard<std::mutex> lock(socketMutex);
                        messageQueue.emplace(std::string(buffer));
                    }
                    messageAvailable.notify_one();
                }
                close(clientSocket);
            }
            else {
                // std::cout << "Error accepting connection\n";
                throw std::runtime_error("Error accepting connection");
            }
        }
    }

    std::string getMessage() {
        std::unique_lock<std::mutex> lock(socketMutex);
        // while (messageQueue.empty()) {
        //     messageAvailable.wait(lock);  
        // }

        messageAvailable.wait(lock, [this]{ return !messageQueue.empty(); });

        auto message = messageQueue.front();
        messageQueue.pop();

        return message;
    }

};

#endif 
