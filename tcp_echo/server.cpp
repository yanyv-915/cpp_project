// server.cpp
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
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket creation failed");
        return 1;
    }

    // 2. 绑定地址和端口
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // host to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // 3. 监听
    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // 4. 接收连接
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept failed");
        close(server_fd);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) break;

        std::cout << "Received: " << buffer;

        // 回显
        write(client_fd, buffer, bytes_read);
    }

    close(client_fd);
    close(server_fd);
    std::cout << "Connection closed.\n";
    return 0;
}
