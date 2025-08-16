#include <iostream>
#include <cstring>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
using namespace std;
const int PORT = 12345;
const int BUF_SIZE = 4096;
const int MAX_SIZE = 1024;
unordered_map<int, string> write_buffer;

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int main()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd==-1){
        perror("listen");
        return -1;
    }
    if(setNonBlocking(listen_fd)==-1){
        perror("fctl");
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr = (INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(bind(listen_fd, (sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("bind");
        return -1;
    }

    if(listen(listen_fd,5)==-1){
        perror("listen");
        return -1;
    }
    int epfd = epoll_create1(0);
    epoll_event ev{}, events[MAX_SIZE];
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    cout << "服务端已启动" << endl;
    while (true)
    {
        int n = epoll_wait(epfd, events, MAX_SIZE, -1);
        if (n == -1)
        {
            perror("epoll_wait");
            return -1;
        }
        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            if (fd == listen_fd)
            {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
                if(client_fd==-1){
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break; // 没有新连接，退出循环
                    else{
                        perror("accept");
                        break;
                    }
                }
                if (setNonBlocking(client_fd) == -1)
                {
                    close(client_fd);
                    continue;
                }
                write_buffer[client_fd] = "";
                epoll_event ev{};
                ev.data.fd = client_fd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                cout << "新客户端" << client_fd << "已连接" << endl;
            }
            else if (events[i].events & EPOLLIN)
            {
                char buffer[BUF_SIZE];
                while (true)
                {
                    int len = read(fd, buffer, sizeof(buffer));
                    if (len == 0)
                    {
                        // 对端关闭
                        cout << "客户端" << fd << "连接已断开" << endl;
                        close(fd);
                        write_buffer.erase(fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        break;
                    }
                    else if (len < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break; // 非阻塞下，读完数据退出循环
                        }
                        else
                        {
                            perror("read");
                            close(fd);
                            write_buffer.erase(fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            break;
                        }
                    }
                    else
                    {
                        string msg(buffer, len);
                        cout << "客户端" << fd << "消息：" << msg << endl;
                        for (auto &[client_fd, buf] : write_buffer)
                        {
                            if (client_fd == fd)
                                continue;
                            buf += msg;
                            epoll_event ev{};
                            ev.data.fd = client_fd;
                            ev.events = EPOLLIN | EPOLLOUT;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &ev);
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                auto &buffer = write_buffer[fd];
                if(!buffer.empty())
                {
                    while (!buffer.empty())
                    {
                        int len = send(fd, buffer.data(), buffer.size(), MSG_NOSIGNAL);
                        if (len > 0)
                        {
                            buffer.erase(0, len);
                        }
                        else
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            else
                            {
                                cout << "客户端" << fd << "写入出错！" << endl;
                                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                                close(fd);
                                write_buffer.erase(fd);
                                break;
                            }
                        }
                    }
                }
                epoll_event ev{};
                ev.data.fd = fd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            }
        }
    }
    close(epfd);
    close(listen_fd);
}