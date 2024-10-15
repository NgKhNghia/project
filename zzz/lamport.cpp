#include "lamport.h"

LamportNode::LamportNode(int id, const std::string& address, int port, std::shared_ptr<Comm> comm, std::shared_ptr<Config> config)
    : Node(id, address, port, comm, config), lamportTimestamp(0) {}

void LamportNode::sendLamportMessage(int receiverId, const std::string& content) {
    std::lock_guard<std::mutex> lock(timestampMutex);
    
    incrementTimestamp();
    int currentTimestamp = getTimestamp();

    std::string lamportMessage = "Timestamp: " + std::to_string(currentTimestamp) + ", Message: " + content;
    sendMessage(receiverId, lamportMessage);
    std::cout << "Sended message to " << receiverId << ": " << lamportMessage << std::endl;
}

void LamportNode::receiveLamportMessage() {
    std::lock_guard<std::mutex> lock(timestampMutex);

    incrementTimestamp();

    std::pair<int, std::string> fullMessage = receiveMessage();
    int receiverId = fullMessage.first;
    std::string message = fullMessage.second;
    
    if (receiverId != -1 && !message.empty()) {
        int commaIndex = message.find(",");
        int timestamp = std::stoi(message.substr(11, commaIndex - 11));
        int lastSpaceIndex = message.find_last_of(" ");
        std::string content = message.substr(lastSpaceIndex + 1);

        lamportTimestamp.store(std::max(lamportTimestamp.load(), timestamp) + 1);
        std::cout << "Received message from " << receiverId << ": " << message << std::endl;
    }
}

int LamportNode::getTimestamp() const {
    std::lock_guard<std::mutex> lock(timestampMutex);
    return lamportTimestamp.load();
}

void LamportNode::incrementTimestamp() {
    std::lock_guard<std::mutex> lock(timestampMutex);
    lamportTimestamp++;
}
