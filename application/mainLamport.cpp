// g++ application/mainLamport.cpp -o application/mainLamport -lpthread -Iframework -Ialgorithm

#include "node.h"
#include "lamport.h"
#include <iostream>
#include <thread>

using namespace std;

Logger logger;
Config config;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Please enter ID\n";
        return 1;
    }

    logger.setMethods(true, true);
    logger.init();

    int id = std::stoi(argv[1]);
    std::string ip = config.getNodeIp(id);
    int port = config.getNodePort(id);
    std::shared_ptr<Comm> comm = std::make_shared<Comm>(port);
    LamportNode lamportNode(id, ip, port, comm);
    lamportNode.initialize();

    std::thread([&] {
        while (1) {
            lamportNode.receiveLamportMessage();
        }
    }).detach();

    std::thread([&] {
        while (1){
            int key;
            std::cin >> key;
            if (key == 1) {
                lamportNode.requestCriticalSection();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (lamportNode.canEnterCriticalSection()) {
                    lamportNode.enterCriticalSection();
                    lamportNode.releaseCriticalSection();
                }
            }
        }
    }).detach(); 
        
    while (1);

    return 0;
}
