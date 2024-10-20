#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::string layThoiGianHienTai() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_r(&time, &localTime); // Sử dụng localtime_r cho Linux

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int main() {
    std::cout << "Thoi gian hien tai: " << layThoiGianHienTai() << "jiji";
    return 0;
}
