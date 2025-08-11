#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <cerrno>
#include<algorithm>
std::atomic<bool> running(true);
std::mutex cout_mutex;        // 控制台输出锁
std::mutex clients_mutex;     // 客户端列表锁
std::vector<int> clients;     // 所有已连接客户端的 socket

// 广播消息给所有客户端
void broadcast_message(const std::string &msg, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int fd : clients) {
        if (fd != sender_fd) { // 不给发送者自己发
            ssize_t sent = write(fd, msg.c_str(), msg.size());
            if (sent == -1) {
                std::lock_guard<std::mutex> lock_cout(cout_mutex);
                std::cerr << "[ERROR] 向客户端写入失败: "
                          << strerror(errno) << " (errno=" << errno << ")\n";
            }
        }
    }
}

// 处理单个客户端
void handle_client(int client_fd, sockaddr_in client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    uint16_t client_port = ntohs(client_addr.sin_port);

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[INFO] 新客户端连接: " << client_ip << ":" << client_port << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_fd);
    }

    char buffer[1024];
    while (running) {
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::string msg = "[客户端 " + std::string(client_ip) + ":" + std::to_string(client_port) + "] " + buffer;

            // 在服务器控制台显示
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << msg << std::endl;
            }

            // 广播给其他客户端
            broadcast_message(msg, client_fd);
        }
        else if (n == 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[INFO] 客户端 " << client_ip << ":" << client_port << " 断开连接" << std::endl;
            break;
        }
        else {
            if (errno == EINTR) continue;
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "[ERROR] read 失败: " << strerror(errno) << " (errno=" << errno << ")\n";
            break;
        }
    }

    close(client_fd);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[ERROR] socket 创建失败: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12345);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "[ERROR] bind 失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "[ERROR] listen 失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "[INFO] 群聊服务器启动，等待连接..." << std::endl;

    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            if (errno == EINTR) continue;
            std::cerr << "[ERROR] accept 失败: " << strerror(errno) << std::endl;
            break;
        }

        std::thread(handle_client, client_fd, client_addr).detach();
    }

    close(server_fd);
    return 0;
}
