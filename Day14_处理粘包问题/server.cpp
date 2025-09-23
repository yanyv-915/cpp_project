#include"MyHeader.h"
#include"ThreadPool.h"
using namespace std;
int epfd{-1};
int efd{-1};
int listen_fd{-1};
int timer_fd{-1};
mutex cout_mtx;
mutex c_mtx;
struct ClientInfo{
    int fd;
    string recv_buf; // 接收缓冲区
    string send_buf; // 发送缓冲区
    time_t last_active;
};
unordered_map<int, ClientInfo> clients;
void add_epoll(int fd){
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    CHECK_ERR(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev), "epoll_ctl error");
}
void mod_epoll(int fd, uint32_t events){
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events | EPOLLET;
    CHECK_ERR(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev), "epoll_ctl mod error");
}
void del_epoll(int fd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    {
        unique_lock<mutex> lock(c_mtx);
        clients.erase(fd);
    }
    lock_guard<mutex> lock(cout_mtx);
    cout << "客户端" << fd << "断开连接" << endl;
}
inline int setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags==-1) return -1;
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK)==-1) return -1;
    return 0;
}
int send_msg(int fd ,const string& msg){
    uint32_t len=htonl(msg.size());
    string packet;
    packet.resize(4 + msg.size());
    memcpy(packet.data(), &len, 4);
    memcpy(packet.data() + 4, msg.data(), msg.size());
    return send(fd, packet.data(), packet.size(), 0);
}
void server_out(int fd, const string& msg){
    lock_guard<mutex> lock(cout_mtx);
    cout << "收到客户端" << fd << "消息: " << msg << endl;
}
void handle_write(int fd){
    unique_lock<mutex> lock(c_mtx);
    auto it = clients.find(fd);
    if(it == clients.end()) return;
    string& buf = it->second.send_buf;
    while(!buf.empty()){
        ssize_t n = send(fd, buf.data(), buf.size(), 0);
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("send_msg error");
            lock.unlock();
            del_epoll(fd);
            return;
        }
        buf.erase(0, n);
    }
    if(buf.empty()) mod_epoll(fd, EPOLLIN);
}
void broadcast_msg(int send_fd, const string& msg){
    lock_guard<mutex> lock(c_mtx);
    for(auto& [fd, client] : clients){
        if(fd == send_fd) continue;
        if(client.send_buf.empty()){
            ssize_t n = send(fd, msg.data(), msg.size(), 0);
            if(n < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    client.send_buf = msg;
                    mod_epoll(fd, EPOLLOUT|EPOLLIN);
                }else{
                    perror("send_msg error");
                    del_epoll(fd);
                }
            }else if(n < msg.size()){
                client.send_buf = msg.substr(n);
                mod_epoll(fd, EPOLLOUT|EPOLLIN);
            }
        }else{
            client.send_buf += msg;
            mod_epoll(fd, EPOLLOUT|EPOLLIN);
        }
    }
}
int main(){
    ThreadPool pool;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERR(listen_fd, "socket error");
    setNonBlocking(listen_fd);
    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    CHECK_ERR(bind(listen_fd, (sockaddr*)&servaddr, sizeof(servaddr)), "bind error");
    CHECK_ERR(listen(listen_fd, 10), "listen error");
    epfd=epoll_create1(0);
    CHECK_ERR(epfd, "epoll_create1 error");
    epoll_event ev{};
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN | EPOLLET;
    CHECK_ERR(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev), "epoll_ctl error");
    cout<<"服务器已成功启动，等待客户端连接..."<<endl;
    efd=eventfd(0, EFD_NONBLOCK);
    CHECK_ERR(efd, "eventfd error");
    setNonBlocking(efd);
    add_epoll(efd);
    vector<epoll_event> events(MAX_EVENT);
    timer_fd=timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    itimerspec new_value{};
    new_value.it_value.tv_sec = HEARTBEAT_INTERVAL;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = HEARTBEAT_INTERVAL;
    new_value.it_interval.tv_nsec = 0;
    CHECK_ERR(timerfd_settime(timer_fd,0,&new_value,nullptr), "timerfd_settime error");
    setNonBlocking(timer_fd);
    add_epoll(timer_fd);
    cout<<listen_fd<<endl;
    while(true){
        int nfds = epoll_wait(epfd, events.data(), MAX_EVENT, -1);
        if(nfds < 0){
            perror("epoll_wait error");
            continue;
        }
        for(int i=0; i<nfds; i++){
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;
            if(fd == listen_fd){
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(fd, (sockaddr*)&client_addr, &client_len);
                if(client_fd == -1) continue;
                if(setNonBlocking(client_fd) == -1) continue;
                add_epoll(client_fd);
                {
                    unique_lock<mutex> lock(c_mtx);
                    clients[client_fd] = {client_fd, "", "", time(nullptr)};
                }
                lock_guard<mutex> lock(cout_mtx);
                cout << "新客户端" << client_fd << "连接成功" << endl;
                continue;
            }
            if(ev & EPOLLIN){
                char buffer[MAX_BUF];
                ssize_t n = 0;
                bool closed = false;
                while(true){
                    n = recv(fd, buffer, sizeof(buffer), 0);
                    if(n > 0){
                        unique_lock<mutex> lock(c_mtx);
                        auto it = clients.find(fd);
                        if(it == clients.end()) break;
                        it->second.recv_buf.append(buffer, n);
                        // 粘包处理
                        while(it->second.recv_buf.size() >= 4){
                            uint32_t msg_len = 0;
                            memcpy(&msg_len, it->second.recv_buf.data(), 4);
                            msg_len = ntohl(msg_len);
                            if(it->second.recv_buf.size() < msg_len + 4) break;
                            string msg = it->second.recv_buf.substr(4, msg_len);
                            it->second.recv_buf.erase(0, msg_len + 4);
                            lock.unlock();
                            // 业务处理和广播
                            pool.enqueue([fd, data=move(msg)](){
                                server_out(fd, data);
                                uint32_t len = htonl(data.size());
                                string header;
                                header.resize(4);
                                memcpy(header.data(), &len, 4);
                                broadcast_msg(fd, header + data);
                            });
                            lock.lock();
                        }
                    }else if(n == 0){
                        closed = true;
                        break;
                    }else{
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("recv error");
                        closed = true;
                        break;
                    }
                }
                if(closed){
                    del_epoll(fd);
                }else{
                    unique_lock<mutex> lock(c_mtx);
                    if(clients.count(fd)) clients[fd].last_active = time(nullptr);
                }
            }
            if(ev & EPOLLOUT){
                pool.enqueue([fd](){ handle_write(fd); });
            }
            if(fd == efd){
                uint64_t one;
                CHECK_ERR(recv(efd, &one, sizeof(one), 0), "recv eventfd error");
            }
            if(fd == timer_fd){
                uint64_t exp;
                recv(timer_fd, &exp, sizeof(exp), 0);
                time_t now = time(nullptr);
                vector<int> to_remove;
                {
                    unique_lock<mutex> lock(c_mtx);
                    for(auto& [fd, client] : clients){
                        if(now - client.last_active > TIMEOUT){
                            to_remove.push_back(fd);
                        }
                    }
                }
                for(int fd : to_remove){
                    lock_guard<mutex> lock(cout_mtx);
                    cout << "客户端" << fd << "超时被踢出" << endl;
                    del_epoll(fd);
                }
            }
        }
    }
    close(timer_fd);
    close(efd);
    close(epfd);
    close(listen_fd);
    return 0;
}