#include<sys/epoll.h>
#include<unistd.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<iostream>
#include<cstring>
#include<vector>
#include<algorithm>
#include<chrono>
#include<ctime>
#include<sys/timerfd.h>
#include<unordered_map>
#include"ThreadPool.h"
using namespace std;
#define PORT 12345
#define MAX_EVENT 1024
#define MAX_BUF 4096
#define TIMEOUT 30
#define HEARTBEAT_INTERVAL 10
#define CHECK_ERR(expr, msg) do { \
    if ((expr) == -1) { \
        perror(msg); \
        exit(EXIT_FAILURE);     \
    } \
} while(0)

mutex cout_mtx;
mutex c_mtx;

struct ClientInfo{
    int fd;
    string msg;
    time_t last_active;
};
unordered_map<int,ClientInfo>clients;
int setNonBlocking(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1) return flags;
    return fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

void add_epoll(int epfd,int fd,uint32_t events){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
    return;
}
void mod_epoll(int epfd,int fd,uint32_t events){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=events|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}
void del_epoll(int epfd,int fd){
    {
        unique_lock<mutex> lock(cout_mtx);
        cout << "客户端" << fd << "断开连接" << endl;
    }
    close(fd);
    {
        unique_lock<mutex> lock(c_mtx);
        clients.erase(fd);
    }
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
    return;
}
void broadcast(int epfd,int send_fd,const string& msg){
    for(auto &[fd,c]:clients){
        if(fd==send_fd) continue;
        epoll_event ev{};
        ev.data.fd=fd;
        ev.events=EPOLLIN|EPOLLOUT|EPOLLET;
        CHECK_ERR(epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev),"epoll_ctl");
        {
            unique_lock<mutex> lock(c_mtx);
            clients[fd].msg += "客户端" + to_string(send_fd) + ":" + msg;
        }
    }
}
void handle_read(int epfd,int client_fd){
    string msg;
    char buffer[MAX_BUF];
    while(true){
        ssize_t n=read(client_fd,buffer,sizeof(buffer));
        if(n==0){
            del_epoll(epfd,client_fd);
        }
        else if(n==-1){
            if(errno==EAGAIN||errno==EWOULDBLOCK) break;
            perror("read");
            del_epoll(epfd,client_fd);
            return;
        }
        else{
            buffer[n]='\0';
            msg+=string(buffer);
        }
    }
    {   
        unique_lock<mutex> lock(cout_mtx);
        cout<<"客户端"<<client_fd<<":"<<msg<<endl;
    }
    broadcast(epfd,client_fd,msg);
    return;
}

void handle_write(int epfd,int client_fd){
    while (!clients[client_fd].msg.empty())
    {
        ssize_t n = write(client_fd, clients[client_fd].msg.data(), (clients[client_fd].msg).size());
        if (n == 0)
        {
            del_epoll(epfd, client_fd);
        }
        else if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            del_epoll(epfd, client_fd);
        }
        else
        {
            unique_lock<mutex> lock(c_mtx);
            clients[client_fd].msg.erase(0, n);
        }
    }
}
int main(){
    ThreadPool pool;
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    CHECK_ERR(listen_fd,"socket");
    CHECK_ERR(setNonBlocking(listen_fd),"fctl");
    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    CHECK_ERR(bind(listen_fd,(sockaddr*)&server_addr,sizeof(server_addr)),"bind");
    CHECK_ERR(listen(listen_fd,4),"listen");

    int epfd=epoll_create1(0);
    vector<epoll_event> events(MAX_EVENT);
    add_epoll(epfd,listen_fd,EPOLLIN);
    cout<<"服务器已启动"<<endl;
    while(true){
        int n=epoll_wait(epfd,events.data(),MAX_EVENT,-1);
        CHECK_ERR(n,"epoll_wait");
        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;
            if(fd==listen_fd){
                sockaddr_in client_addr{};
                socklen_t client_len=sizeof(client_addr);
                int client_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);
                if(client_fd==-1) continue;
                if(setNonBlocking(client_fd)==-1) continue;
                add_epoll(epfd,client_fd,EPOLLIN);
                {
                    unique_lock<mutex> lock(c_mtx);
                    clients[client_fd] = {client_fd, "", time(0)};
                }
            }
            else if(events[i].events & EPOLLIN){
                pool.enqueue([epfd,fd](){
                    handle_read(epfd,fd);
                });
            }
            else if(events[i].events & EPOLLOUT){
                pool.enqueue([epfd,fd](){
                    handle_write(epfd,fd);
                });
                if(clients[fd].msg.empty()){
                    mod_epoll(epfd,fd,EPOLLIN);
                }
            }
        }
    }
    close(epfd);
    close(listen_fd);
}
