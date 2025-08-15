
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <cstring> 
#include <iostream> 
#include <thread> 
#include <vector> 
#include <mutex> 
#include <algorithm> 
#include <cerrno> 
#include"ThreadPool.h"
std::vector<int> clients; 
std::mutex clients_mutex; 
void broadcast(const std::string &msg, int sender_fd) 
{ std::lock_guard<std::mutex> lock(clients_mutex); 
    for (int client_fd : clients) 
    { if (client_fd != sender_fd) 
        { ssize_t ret = write(client_fd, msg.c_str(), msg.size()); 
            if (ret == -1) { std::cerr << "[ERROR] write失败: " << strerror(errno) << " (errno=" << errno << ")\n"; 
            } 
        } 
    } 
}
void handle_client(int conn_fd)
{
    char buffer[1024];
    while (true)
    {
        ssize_t n = read(conn_fd, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            std::string msg = "客户端[" + std::to_string(conn_fd) + "]: " + buffer;
            std::cout << msg<<std::endl;
            broadcast(msg, conn_fd);
        }
        else if (n == 0)
        {
            std::cout << "[INFO] 客户端 " << conn_fd << " 断开连接\n";
            break;
        }
        else
        {
            std::cerr << "[ERROR] read失败: " << strerror(errno) << " (errno=" << errno << ")\n";
            break;
        }
    } // 安全移除客户端
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), conn_fd), clients.end());
    }
    close(conn_fd);
    std::cout << "[INFO] 关闭连接 fd=" << conn_fd << std::endl;
}
int main()
{
    ThreadPool pool(4);
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        std::cerr << "[FATAL] socket创建失败: " << strerror(errno) << std::endl;
        return 1;
    }
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12345);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        std::cerr << "[FATAL] bind失败: " << strerror(errno) << std::endl;
        return 1;
    }
    if (listen(listen_fd, 5) == -1)
    {
        std::cerr << "[FATAL] listen失败: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << "[INFO] 服务器启动，等待连接...\n";
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
        if (conn_fd == -1)
        {
            std::cerr << "[ERROR] accept失败: " << strerror(errno) << " (errno=" << errno << ")\n";
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "[INFO] 新客户端连接: " << client_ip << ":" << ntohs(client_addr.sin_port) << " (fd=" << conn_fd << ")\n";
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(conn_fd);
        }
        pool.enqueue([conn_fd]{
            handle_client(conn_fd);
        });
    }
    close(listen_fd);
    return 0;
}