#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
using namespace std;

int main() {
    // 创建套接字（AF_INET 表示 IPv4，SOCK_STREAM 表示 TCP）
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 配置服务器地址结构（使用 IPv4，端口 12345）
    sockaddr_in c_addr{};
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(12345);  // 主机字节序转网络字节序

    // 将字符串形式的 IP 地址转换为网络字节序的二进制形式
    inet_pton(AF_INET, "127.0.0.1", &c_addr.sin_addr.s_addr);

    // 发起连接请求，连接目标服务器
    connect(sock_fd, (sockaddr*)&c_addr, sizeof(c_addr));
    cout << "连接成功！" << endl;

    // 关闭套接字，断开连接
    close(sock_fd);
    return 0;
}
