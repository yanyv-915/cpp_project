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
void del_epoll(int epfd,int fd){
    close(fd);
    clients.erase(fd);
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
        clients[fd].msg+="客户端"+to_string(send_fd)+":"+msg;
    }
}
int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    CHECK_ERR(listen_fd,"listen");
    CHECK_ERR(setNonBlocking(listen_fd),"fctl");
    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(listen_fd,(sockaddr*)&server_addr,sizeof(server_addr))==-1){
        perror("bind");
        return -1;
    }
    if(listen(listen_fd,5)==-1){
        perror("listen");
        return -1;
    }

    int epfd=epoll_create1(0);
    epoll_event events[MAX_EVENT];
    add_epoll(epfd,listen_fd,EPOLLIN);

    int timer_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    itimerspec new_value{};
    new_value.it_interval.tv_sec=HEARTBEAT_INTERVAL;
    new_value.it_value.tv_sec=HEARTBEAT_INTERVAL;
    timerfd_settime(timer_fd,0,&new_value,nullptr);
    add_epoll(epfd,timer_fd,EPOLLIN);

    cout<<"服务端已启动！"<<endl;
    while(true){
        int n=epoll_wait(epfd,events,MAX_EVENT,-1);
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
                clients[fd]={fd,"",time(0)};
                cout<<"新客户端"<<client_fd<<"已连接"<<endl;
            }
            else if(fd==timer_fd){
                uint64_t exp;
                read(fd,&exp,sizeof(exp));
                time_t now=time(0);
                vector<int>to_close;
                for(auto [fd,c]:clients){
                    if(now-c.last_active>TIMEOUT) {
                        to_close.push_back(fd);
                        cout<<"客户端"<<fd<<"已超时断开连接！"<<endl;
                    }
                }
                for(int &fd:to_close){
                    del_epoll(epfd,fd);
                }
            }
            else if(events[i].events & EPOLLIN){
                char buffer[MAX_BUF];
                string msg="";
                while(true){
                    memset(buffer,0,sizeof(buffer));
                    int count=read(fd,buffer,sizeof(buffer));
                    if(count==0){
                        cout<<"客户端"<<fd<<"断开连接"<<endl;
                        del_epoll(epfd,fd);
                        break;
                    }
                    else if(count==-1){
                        if(errno==EAGAIN||errno==EWOULDBLOCK) break;
                        else{
                            perror("read");
                            cout << "客户端" << fd << "断开连接" << endl;
                            del_epoll(epfd,fd);
                            break;
                        }
                    }
                    else{
                        buffer[count]='\0';
                        msg+=string(buffer);
                    }
                    clients[fd].last_active=time(0);
                    if(msg=="PING"){
                        write(fd,"PONG",4);
                    }
                    else{
                        cout << "客户端" << fd << ":" << msg << endl;
                        broadcast(epfd, fd, msg);
                    }
                }
            }
            else if(events[i].events & EPOLLOUT){
                while(!clients[fd].msg.empty()){
                    ssize_t count=write(fd,clients[fd].msg.c_str(),sizeof(clients[fd].msg));
                    if(count==-1){
                        perror("write");
                        break;
                    }
                    clients[fd].msg.erase(0,count);
                }
                if(clients[fd].msg.empty()){
                    epoll_event ev{};
                    ev.data.fd = fd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                }
                else{
                    del_epoll(epfd,fd);
                    cout<<"客户端"<<fd<<"异常！"<<endl;
                }
            }
            else{

            }
        }
    }
    close(epfd);
    close(listen_fd);
    return 0;
}