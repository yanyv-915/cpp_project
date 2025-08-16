#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <cstring>
#include<iostream>
void read_server(int fd) {
    char buffer[1024];
    while (true) {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << buffer << std::endl;
        } else if (n == 0) {
            std::cout << "Server closed connection\n";
            break;
        } else {
            perror("read");
            break;
        }
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        return 1;
    }
    std::cout<<"服务端成功连接"<<std::endl;

    std::thread reader(read_server, sock_fd);
    reader.detach();

    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        if (msg == "exit") break;

        ssize_t n = write(sock_fd, msg.c_str(), msg.size());
        if (n == -1) {
            perror("write");
            break;
        }
    }

    close(sock_fd);
    return 0;
}
