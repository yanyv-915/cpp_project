#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <cerrno>

std::atomic<bool> running(true);

void recv_thread_func(int sockfd) {
    char buffer[1024];
    while (running) {
        ssize_t n = read(sockfd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << "[服务器] " << buffer << std::endl;
        } 
        else if (n == 0) {
            std::cout << "[INFO] 服务器已关闭连接\n";
            running = false;
            break;
        } 
        else {
            if (errno == EINTR) continue; // 信号中断，继续读
            std::cerr << "[ERROR] 客户端 read 失败: " 
                      << strerror(errno) << " (errno=" << errno << ")\n";
            running = false;
            break;
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "socket 创建失败: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << "[DEBUG] 创建 socket fd = " << sockfd << std::endl;

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "connect 失败: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }
    std::cout << "[DEBUG] 连接成功 sockfd = " << sockfd << std::endl;

    // 启动接收线程
    std::thread recv_thread(recv_thread_func, sockfd);

    // 主线程发送数据
    std::string msg;
    while (running) {
        std::getline(std::cin, msg);
        if (msg == "quit") {
            running = false;
            break;
        }
        ssize_t sent = write(sockfd, msg.c_str(), msg.size());
        if (sent == -1) {
            std::cerr << "[ERROR] 客户端 write 失败: " 
                      << strerror(errno) << " (errno=" << errno << ")\n";
            running = false;
            break;
        }
    }

    // 等待接收线程退出
    if (recv_thread.joinable()) recv_thread.join();

    // 最后再安全关闭 socket
    std::cout << "[DEBUG] 关闭 sockfd = " << sockfd << std::endl;
    close(sockfd);
    return 0;
}
