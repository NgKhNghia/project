#ifndef LAMPORT_H
#define LAMPORT_H

#include "node.h"
#include <atomic>
#include <mutex>
#include <queue>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

enum MessageType { REQUEST, REPLY, RELEASE }; // Định nghĩa các loại tin nhắn

struct Message {
    int senderId;            // ID của nút gửi tin nhắn
    int timestamp;           // Dấu thời gian của tin nhắn
    MessageType type;        // Loại tin nhắn (REQUEST, REPLY, hoặc RELEASE)
    std::string content;     // Nội dung tin nhắn
};

class LamportNode : public Node {
private:
    std::atomic<int> lamportTimestamp;  // Đồng hồ logic của Lamport
    std::priority_queue<Message, std::vector<Message>, bool(*)(const Message&, const Message&)> requestQueue; // Hàng đợi yêu cầu
    std::mutex queueMutex;  
    std::map<int, bool> replyReceived;  // Theo dõi trạng thái nhận REPLY từ các nút khác

    static bool compareMessages(const Message &m1, const Message &m2) {
        // So sánh các tin nhắn theo dấu thời gian và ID của nút
        return (m1.timestamp > m2.timestamp) || 
               (m1.timestamp == m2.timestamp && m1.senderId > m2.senderId);
    }

public:
    LamportNode(int id, const std::string& ip, int port, std::shared_ptr<Comm> comm, std::shared_ptr<Config> config)
        : Node(id, ip, port, comm, config), lamportTimestamp(0), requestQueue(compareMessages) {}

    int getTimestamp() const {
        return lamportTimestamp.load();
    }

    // Gửi tin nhắn với loại MessageType và nội dung tương ứng
    void sendLamportMessage(int receiverId, MessageType type, const std::string& content = "") {
        incrementTimestamp();
        int currentTimestamp = getTimestamp();
        
        Message msg = {getId(), currentTimestamp, type, content};
        std::string messageContent = formatMessage(msg);
        
        sendMessage(receiverId, messageContent);
    }

    // Xử lý tin nhắn Lamport nhận được, cập nhật đồng hồ và hàng đợi
    void receiveLamportMessage() {
        std::string messageContent = receiveMessage();
        Message msg = parseMessage(messageContent);
        int senderTimestamp = msg.timestamp;
        int senderId = msg.senderId;

        // Cập nhật đồng hồ Lamport thành max(đồng hồ hiện tại, dấu thời gian nhận được) + 1
        incrementTimestamp();
        lamportTimestamp.store(std::max(lamportTimestamp.load(), senderTimestamp) + 1);

        std::lock_guard<std::mutex> lock(queueMutex);
        switch (msg.type) {
            case REQUEST:
                // Thêm yêu cầu vào hàng đợi và gửi REPLY
                requestQueue.push(msg);
                sendLamportMessage(senderId, REPLY, "OK");
                break;
                
            case REPLY:
                // Đánh dấu đã nhận REPLY từ nút này
                replyReceived[senderId] = true;
                break;

            case RELEASE:
                // Xoá yêu cầu của nút gửi tin nhắn khỏi hàng đợi
                removeRequest(senderId);
                break;
        }
    }

    // Khởi tạo yêu cầu vào vùng găng (critical section)
    void requestCriticalSection() {
        incrementTimestamp();
        int currentTimestamp = getTimestamp();

        // Thêm yêu cầu của nút vào hàng đợi
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            Message request = {getId(), currentTimestamp, REQUEST, "Requesting CS"};
            requestQueue.push(request);
            resetReplies();
        }

        // Phát đi tin nhắn REQUEST đến tất cả các nút khác
        for (const auto& node : getNodeConfigs()) {
            if (node.first != getId()) {
                sendLamportMessage(node.first, REQUEST);
            }
        }
    }

    // Kiểm tra nếu nút có thể vào vùng găng hay không
    bool canEnterCriticalSection() {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        // Điều kiện để vào vùng găng
        return std::all_of(replyReceived.begin(), replyReceived.end(), 
                [](const std::pair<int, bool>& entry) { return entry.second; }) &&
                !requestQueue.empty() && requestQueue.top().senderId == getId();
    }

    // Thoát khỏi vùng găng và thông báo cho các nút khác
    void releaseCriticalSection() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            // Xóa yêu cầu của nút khỏi hàng đợi
            if (!requestQueue.empty() && requestQueue.top().senderId == getId()) {
                requestQueue.pop();
            }
        }

        // Phát đi tin nhắn RELEASE đến tất cả các nút khác
        for (const auto& node : getNodeConfigs()) {
            if (node.first != getId()) {
                sendLamportMessage(node.first, RELEASE);
            }
        }
    }

private:
    void incrementTimestamp() {
        lamportTimestamp++;
    }

    void resetReplies() {
        // Thiết lập lại trạng thái nhận REPLY cho yêu cầu mới
        for (auto& entry : replyReceived) {
            entry.second = false;
        }
    }

    // Phân tích nội dung tin nhắn nhận được thành cấu trúc Message
    Message parseMessage(const std::string& messageContent) {
        // "Id: 1, Timestamp: 5, Type: REQUEST, Content: Requesting CS"
        int senderIdIndex = messageContent.find("Id: ");
        int senderTimestampIndex = messageContent.find("Timestamp: ");
        int senderTypeIndex = messageContent.find("Type: ");
        int senderContentIndex = messageContent.find("Content: ");

        int senderId = std::stoi(messageContent.substr(senderIdIndex + 4, senderTimestampIndex - (senderIdIndex + 4) - 2));
        int senderTimestamp = std::stoi(messageContent.substr(senderTimestampIndex + 11, senderTypeIndex - (senderTimestampIndex + 11) - 2));
        std::string tmp = messageContent.substr(senderTypeIndex + 6, senderContentIndex - (senderTypeIndex + 6) - 2); 
        MessageType senderType = (tmp == "REQUEST") ? REQUEST : (tmp == "REPLY") ? REPLY : RELEASE;
        std::string senderContent = messageContent.substr(senderContentIndex + 9);

        Message msg = {senderId, senderTimestamp, senderType, senderContent};
        return msg;
    }

    // Định dạng cấu trúc Message thành chuỗi tin nhắn
    std::string formatMessage(const Message& msg) {
        std::string typeStr = (msg.type == REQUEST) ? "REQUEST" : (msg.type == REPLY) ? "REPLY" : "RELEASE";
        return "Id: " + std::to_string(msg.senderId) + ", Timestamp: " + std::to_string(msg.timestamp) + 
               ", Type: " + typeStr + ", Content: " + msg.content;
    }

    // Xoá yêu cầu của một nút cụ thể khỏi hàng đợi
    void removeRequest(int senderId) {
        std::priority_queue<Message, std::vector<Message>, 
            bool(*)(const Message&, const Message&)> tempQueue(compareMessages);
        
        // Di chuyển tất cả tin nhắn ngoại trừ tin nhắn của nút cần xoá vào hàng đợi tạm
        while (!requestQueue.empty()) {
            Message msg = requestQueue.top();
            requestQueue.pop();
            if (msg.senderId != senderId) {
                tempQueue.push(msg);
            }
        }
        
        requestQueue = std::move(tempQueue);
    }
};

#endif // LAMPORT_H
