// client.cpp
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

int main() {
    // 1. 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket creation failed");
        return 1;
    }

    // 2. 设置服务器地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 3. 连接服务器
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock);
        return 1;
    }

    std::cout << "Connected to server.\n";

    char buffer[BUFFER_SIZE];
    while (true) {
        std::cout << "Enter message: ";
        std::cin.getline(buffer, BUFFER_SIZE);

        if (strcmp(buffer, "exit") == 0) break;

        // 发送
        write(sock, buffer, strlen(buffer));

        // 接收回显
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = read(sock, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) break;

        std::cout << "Echoed from server: " << buffer << "\n";
    }

    close(sock);
    std::cout << "Disconnected.\n";
    return 0;
}
