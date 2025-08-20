#include<unistd.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<iostream>
#include<cstring>
#include<unordered_map>
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/timerfd.h>
#include<ctime>
#include"ThreadPool.h"
using namespace std;
#define PORT 12345
#define MAX_BUF 4096
#define MAX_EVENT 1024
#define CHECK_ERR(exp,msg)\
    do{\
        if((exp)==-1){\
            perror(msg);\
            exit(EXIT_FAILURE);\
        }    \
    }\
    while(0)
struct Clients{
    int fd;
    string msg;
    time_t last_active;
};
unordered_map<int,Clients> clients;
mutex c_mtx;
mutex o_mtx;
queue<Clients> tasks;
queue<int>to_del;
int setNonBliocking(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1) return -1;
    return fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}
void add_epoll(int epfd,int fd){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
}
void mod_epoll(int epfd,int fd,uint32_t event){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=event|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}
string processing(int fd,const string& msg){
    {
        unique_lock<mutex> lk(o_mtx);
        cout<<"客户端"+to_string(fd)+":"+msg<<endl;
    }
    return "客户端"+to_string(fd)+":"+msg;
}
void enqueue_out(int fd,string msg,int efd){
    {
        unique_lock<mutex> lk(c_mtx);
        tasks.push({fd, msg});
    }
    uint64_t one=1;
    write(efd,&one,sizeof(one));
}
void broadcast(int epfd,int send_fd,const string& msg){
    for (auto &[fd, c] : clients)
    {
        if (fd == send_fd)
            continue;
        if (clients[fd].msg.empty())
        {
            ssize_t n = write(fd, msg.data(), msg.size());
            if (n == -1 || n == 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    clients[fd].msg = msg;
                    mod_epoll(epfd, fd, EPOLLIN | EPOLLOUT);
                }
                else
                {
                    perror("write");
                    lock_guard<mutex> lk(c_mtx);
                    to_del.emplace(fd);
                }
            }
            else if (n < msg.size())
            {
                clients[fd].msg.assign(msg.begin() + n, msg.end());
                mod_epoll(epfd, fd, EPOLLIN | EPOLLOUT);
            }
        }
        else{
            clients[fd].msg+=msg;
            mod_epoll(epfd,fd,EPOLLIN|EPOLLOUT);
        }
    }
}
void to_one(int epfd,int send_fd,int to_fd,const string& msg){
    {
        unique_lock<mutex> lk(o_mtx);
        cout <<"客户端"<<to_fd<<":"<<msg<<endl;
    }
    if(msg=="PING"){
        ssize_t n=write(to_fd,"PONG",4);
        return;
    }
    ssize_t n = write(to_fd, msg.data(), msg.size());
    if (n == -1 || n == 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            clients[to_fd].msg = msg;
            mod_epoll(epfd, to_fd, EPOLLIN | EPOLLOUT);
        }
        else
        {
            perror("write");
            lock_guard<mutex> lk(c_mtx);
            to_del.emplace(to_fd);
        }
    }
    else if (n < msg.size())
    {
        clients[to_fd].msg.assign(msg.begin() + n, msg.end());
        mod_epoll(epfd, to_fd, EPOLLIN | EPOLLOUT);
    }
}
int main(){
    ThreadPool pool;
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    CHECK_ERR(listen_fd,"listen");
    CHECK_ERR(setNonBliocking(listen_fd),"fcntl");
    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    CHECK_ERR(bind(listen_fd,(sockaddr*)&server_addr,sizeof(server_addr)),"bind");
    CHECK_ERR(listen(listen_fd,5),"listen");
    int epfd=epoll_create1(0);
    add_epoll(epfd,listen_fd);
    int efd=eventfd(0,EFD_NONBLOCK);
    add_epoll(epfd,efd);
    int timer_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    itimerspec new_value{};
    new_value.it_interval.tv_sec=30;
    new_value.it_value.tv_sec=30;
    timerfd_settime(timer_fd,0,&new_value,nullptr);
    add_epoll(epfd,timer_fd);

    vector<epoll_event> events(MAX_EVENT);
    cout<<"服务器已启动"<<endl;
    while(true){
        int n=epoll_wait(epfd,events.data(),MAX_EVENT,-1);
        if(n==-1){
            if(errno==EINTR) continue;
            else{
                perror("epoll_wait");
                break;
            }
        }
        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;
            uint32_t ev=events[i].events;
            if(fd==listen_fd){
                sockaddr_in client_addr{};
                socklen_t client_len=sizeof(client_addr);
                int client_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);
                if(client_fd==-1) continue;
                if(setNonBliocking(client_fd)==-1) continue;
                add_epoll(epfd,client_fd);
                {
                    unique_lock<mutex> lk(o_mtx);
                    cout<<"新客户端"<<client_fd<<"连接"<<endl;
                }
                {
                    unique_lock<mutex> lk(c_mtx);
                    clients[client_fd]={client_fd,"",time(0)};
                }
                continue;
            }
            if(fd==efd){
                uint64_t exp;
                read(efd,&exp,sizeof(exp));
                lock_guard<mutex> lk(c_mtx);
                while (!tasks.empty()){
                    auto [send_fd, msg, t] = move(tasks.front());
                    tasks.pop();
                    if (clients.find(send_fd) == clients.end()) continue;
                    broadcast(epfd, send_fd, msg);
                }
                continue;
            }
            
            if(ev & EPOLLIN){
                char buffer[MAX_BUF];
                string msg="";
                while(true){
                    memset(buffer,0,sizeof(buffer));
                    ssize_t count=read(fd,buffer,sizeof(buffer));
                    if(count==-1){
                        if(errno==EAGAIN||errno==EWOULDBLOCK) break;
                        else{
                            perror("read");
                            break;
                        }
                    }
                    else if(count==0){
                        lock_guard<mutex> lk(c_mtx);
                        to_del.emplace(fd);
                        break;
                    }
                    else{
                        msg+=string(buffer,count);
                    }
                }
                if (!msg.empty()){
                    {
                        unique_lock<mutex> lk(c_mtx);
                        clients[fd].last_active = time(0);
                    }
                    pool.enqueue([=]() mutable{
                        string data=move(msg);
                        if(data=="PING"){
                            to_one(epfd,listen_fd,fd,data);
                        }
                        else{
                            string res = processing(fd, data);
                            enqueue_out(fd, move(res), efd);
                        }
                    });
                }
            }
            if(ev & EPOLLOUT){
                string &msg=clients[fd].msg;
                while(!msg.empty()){
                    ssize_t count=write(fd,msg.data(),msg.size());
                    if (count == -1 || count == 0){
                        if (errno == EAGAIN || errno == EWOULDBLOCK)  break;
                        else{
                            perror("write");
                            lock_guard<mutex> lk(c_mtx);
                            to_del.emplace(fd);
                        }
                    }
                    else {
                        msg.erase(0,count);
                    }
                }
                if(msg.empty()){
                    mod_epoll(epfd,fd,EPOLLIN);
                }
            }
            if(fd==timer_fd){
                uint64_t exp;
                read(fd,&exp,sizeof(exp));
                time_t now=time(0);
                {
                    unique_lock<mutex> lk(c_mtx);
                    for(auto &c:clients){
                        if(now-c.second.last_active>30) {
                            to_del.emplace(c.second.fd);
                        }
                        else{
                            c.second.last_active=now;
                        }
                    }
                }
                continue;
            }
        }
        {
            unique_lock<mutex> lk(c_mtx);
            while(!to_del.empty()){
                int fd=move(to_del.front());
                if(clients.find(fd)==clients.end()) continue;
                close(fd);
                clients.erase(fd);
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
                lock_guard<mutex> lk(o_mtx);
                cout<<"客户端"<<fd<<"已关闭"<<endl;
            }
        }
    }
}