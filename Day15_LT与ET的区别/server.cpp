#include"MyHeader.h"
#include"Logger.h"
using namespace std;
int epfd{-1};
int efd{-1};
struct Client{
    string recv_msg;
    string send_msg;
    time_t last_active;
};
unordered_map<int,Client> clients;
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
void add_epoll(int fd){
    epoll_event ev{};
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
}
int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    bind(listen_fd,(sockaddr*)&server_addr,sizeof(server_addr));
    listen(listen_fd,5);
    epfd=epoll_create1(0);
    add_epoll(listen_fd);
    set_nonblocking(listen_fd);
    vector<epoll_event> events(MAX_EVENT);
    Logger::instance().log(Logger::INFO,"服务端已启动，等待连接。。。");
    while(true){
        int nfds=epoll_wait(epfd,events.data(),MAX_EVENT,-1);
        if(nfds==-1) {
            Logger::instance().log(Logger::ERROR,"error epoll_wait\n");
            continue;
        }
        for(int i=0;i<nfds;i++){
            int fd=events[i].data.fd;
            uint32_t ev=events[i].events;
            if(fd==listen_fd){
                sockaddr_in client_addr{};
                socklen_t client_len=sizeof(client_addr);
                int client_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);
                if(client_fd==-1){
                    Logger::instance().log(Logger::WARING,"warning accept\n");
                    continue;
                }
                if(set_nonblocking(client_fd)==-1){
                    Logger::instance().log(Logger::WARING,"warning nonblock\n");
                    continue;
                }
                add_epoll(client_fd);
                clients[fd]={"","",time(nullptr)};
                continue;
            }
            if(ev & EPOLLIN){
                char buffer[MAX_BUF];
                while(true){
                    ssize_t n=recv(fd,buffer,sizeof(buffer),0);
                    if(n==-1){
                        if(errno==EAGAIN||errno==EWOULDBLOCK) continue;
                        perror("recv");
                        break;
                    }
                    else if(n==0){
                        Logger::instance().log(Logger::INFO,"客户端断开连接\n");
                        break;
                    }
                    else{
                        clients[fd].send_msg+=string(buffer,n);
                    }
                }
                
            }
        }
    }
}