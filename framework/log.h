#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <list>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <sstream>
#include <iomanip>

class LoggingMethod {
public:
    virtual ~LoggingMethod() {}
    virtual void init() {}
    virtual void clean() {}
    virtual void log(const std::string& s) = 0;
    
    std::string getCurrentTime() {  // Lấy thời gian hiện tại để thêm vào log
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;
        localtime_r(&time, &localTime); // Sử dụng localtime_r cho Linux

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S ");
        
        return oss.str();
    }

    std::string formatMessage(const std::string& s) {
        return getCurrentTime() + s + "\n";
    }
};

class ConsoleLoggingMethod : public LoggingMethod {
public:
    void log(const std::string& s) override {
        std::cout << formatMessage(s) << std::endl;
    }
};

class FileLoggingMethod : public LoggingMethod {
protected:
    std::ofstream file;
    std::list<std::string> queue;
    bool closed = false;
    std::mutex mutex;
    std::condition_variable cond;

public:
    void init() override {
        if (file.is_open()) file.close();
        file.open("log.txt", std::ios::out | std::ios::app);
        if (!file) {
            std::cout << "Failed to create file log\n" << std::endl;
            return;
        }
        std::thread([this]() {
            do {
                std::unique_lock lock(mutex);
                cond.wait(lock, [this]() { return !queue.empty() || closed; });

                while (!queue.empty()) {
                    auto& s = queue.front();
                    file << s << std::endl;
                    queue.pop_front();
                }
            } while (!closed);
        }).detach();
    }

    ~FileLoggingMethod() override {
        closed = true;
        cond.notify_one();
    }

    void clean() override {
        if (file.is_open()) file.close();
    }

    void log(const std::string& s) override {
        if (!file.is_open()) return;
        std::unique_lock lock(mutex);
        queue.push_back(formatMessage(s));
        cond.notify_one();
    }
};

class Logger {
protected:
    bool toConsole = true;
    bool toFile = true;
    std::list<std::shared_ptr<LoggingMethod>> methods;

public:
    void setMethods(bool console, bool file) {
        toConsole = console;
        toFile = file;
    }

    void init() {
        if (toConsole) methods.push_back(std::make_shared<ConsoleLoggingMethod>());
        if (toFile) methods.push_back(std::make_shared<FileLoggingMethod>());
        reset();
    }

    void reset() {
        for (auto& m : methods) {
            m->init();
        }
    }

    void log(std::string msg) {
        for (auto& m : methods) {
            m->log(msg);
        }
    }
};

extern Logger logger;






#endif 