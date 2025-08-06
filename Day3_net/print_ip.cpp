#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

int main() {
    // 创建套接字，AF_INET 表示 IPv4，SOCK_STREAM 表示 TCP
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 配置服务器地址结构（使用 IPv4，本地任意地址，端口 12345）
    sockaddr_in se_addr{};
    se_addr.sin_family = AF_INET;
    se_addr.sin_port = htons(12345);                  // 设置端口（主机字节序转网络字节序）
    se_addr.sin_addr.s_addr = htonl(INADDR_ANY);      // 绑定到所有可用网络接口
    socklen_t se_len = sizeof(se_addr);

    // 将套接字绑定到指定的 IP 和端口
    bind(listen_fd, (sockaddr*)&se_addr, se_len);

    // 监听连接请求，最多允许的等待连接数为 5
    listen(listen_fd, 5);

    // 定义客户端地址结构，用于保存客户端信息
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    // 接受客户端连接请求（阻塞），成功返回新的套接字描述符
    int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);

    // 提取并转换客户端 IP 地址为字符串形式
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    cout << "客户端地址: " << client_ip << endl;

    // 关闭连接和监听套接字
    close(conn_fd);
    close(listen_fd);
    return 0;
}
