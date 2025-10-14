#pragma once
#include"MyHeader.h"
#define MAX_BUF 4096
#define MAX_SIZE 1024
using namespace std;
class Server{
private:
    struct client{
        int fd;
        std::string msg;
        time_t last_active;
    };
    struct task{
        int fd;
        string msg;
    };
    int PORT;
    int LEN;
    int listen_fd;
    int epfd;
    int efd;
    int timer_fd;
    int HEART_BEAT;
    int TIME_OUT;
    mutex ct_mtx;
    ThreadPool pool;
    unordered_map<int,client>clients;
    vector<int>to_del;
    queue<task>tasks;
    inline int setNonBlocking(int fd){
        int flag = fcntl(fd, F_GETFL, 0);
        if (flag == -1)
        {
            return flag;
        }
        return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    }
    void set_Timer_fd(){
        timer_fd=timerfd_create(CLOCK_MONOTONIC,O_NONBLOCK);
        itimerspec new_value{};
        new_value.it_value.tv_sec=HEART_BEAT;
        new_value.it_interval.tv_sec=HEART_BEAT;
        timerfd_settime(timer_fd,0,&new_value,nullptr);
        add_epoll(timer_fd);
    }
    void set_Listen_fd(){
        listen_fd=socket(AF_INET,SOCK_STREAM,0);
        CHECK_ERR(listen_fd,"listen");
        setNonBlocking(listen_fd);
        int opt=1;
        setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
        sockaddr_in server_addr{};
        server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(PORT);
        CHECK_ERR(bind(listen_fd,(sockaddr*)&server_addr,sizeof(server_addr)),"bind");
        add_epoll(listen_fd);
    }
    void set_efd(){
        efd=eventfd(0,EFD_NONBLOCK);
        add_epoll(efd);
    }
    void del_fd(int fd){
        lock_guard<mutex> lk(ct_mtx);
        clients.erase(fd);
        del_epoll(fd);
        Logger::instance().log(Logger::INFO,"客户端"+to_string(fd)+"异常断开");
    }
    void add_epoll(int fd){
        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    void mod_epoll(int fd, uint32_t event){
        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = event | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
    void del_epoll(int fd){
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }
    void enqueue_out(int from_fd){
        lock_guard<mutex> lk(ct_mtx);
        MsgHeader header{};
        string& c_msg=clients[from_fd].msg;
        while(c_msg.size()>sizeof(header)){
            memset(&header,0,sizeof(header));
            memcpy(&header,c_msg.data(),sizeof(header));
            uint16_t type=ntohs(header.MsgType);
            uint32_t len=ntohl(header.length);
            if(c_msg.size()<len+sizeof(header)) break;
            string real_msg;
            real_msg.resize(len);
            memcpy(real_msg.data(),c_msg.data()+sizeof(header),len);
            tasks.push({from_fd,c_msg.substr(0,sizeof(header)+len)});
            c_msg.erase(0,sizeof(header)+len);
            //if(real_msg=="PING") continue;
            Logger::instance().log(Logger::INFO,"客户端"+to_string(from_fd)+":"+real_msg);
        }
        uint64_t one=1;
        write(efd,&one,sizeof(one));
    }
    void handle_To_one(){
        
    }
    void handle_Broadcast(int from_fd,const string& msg){
        {
            unique_lock<mutex> lk(ct_mtx);
            for(auto& [fd,c]:clients){
                if(fd==from_fd) continue;
                if(clients.find(fd)==clients.end()) continue;
                c.msg+=msg;
                ssize_t n=write(fd,c.msg.data(),c.msg.size());
                if(n==-1){
                    if(errno==EAGAIN||errno==EWOULDBLOCK) continue;
                    perror("write");
                    clients.erase(fd);
                    del_epoll(fd);
                    Logger::instance().log(Logger::ERRNO,"客户端"+to_string(fd)+"异常断开");
                    continue;
                }
                else if(n==0){
                    clients.erase(fd);
                    del_epoll(fd);
                    Logger::instance().log(Logger::INFO,"客户端"+to_string(fd)+"断开");
                    continue;
                }
                else{
                    c.msg.erase(0,n);
                }
                if(c.msg.empty()) continue;
                mod_epoll(fd,EPOLLIN|EPOLLOUT);
            }
        }
    }
    void handle_Heart_beat(int from_fd){
        std::string pong = packMsg(HEART_BEAT, "PONG");
        ssize_t n = send(from_fd, pong.data(), pong.size(), MSG_NOSIGNAL);
        if (n < 0 && errno != EAGAIN){
            del_fd(from_fd);
            Logger::instance().log(Logger::ERRNO, "心跳包发送失败");
        }
    }
    void handle_task(){
        {
            unique_lock<mutex> lk(ct_mtx);
            while(!tasks.empty()){
                auto t=tasks.front();
                tasks.pop();
                lk.unlock();
                MsgHeader header{};
                memcpy(&header,t.msg.data(),sizeof(header));
                uint16_t type=ntohs(header.MsgType);
                switch (type){
                    case To_one:    
                        handle_To_one();
                        break;
                    case Broadcast:
                        handle_Broadcast(t.fd,t.msg);
                        break;
                    case Heart_beat:
                        handle_Heart_beat(t.fd);
                        break;
                    default:
                        break;
                }
                lk.lock();
            }
        }

    }
    void handle_new_connect(){
        while (true)
        {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
            if (client_fd == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)  break;
                perror("accept");
                break;
            }
            setNonBlocking(client_fd);
            auto now = time(0);
            clients[client_fd]={client_fd,"",now};
            add_epoll(client_fd);
            Logger::instance().log(Logger::INFO, "客户端" + to_string(client_fd) + "成功连接");
        }
    }
    
    void handle_read(int fd){
        char buffer[MAX_BUF];
        while(true){
            memset(buffer,0,sizeof(buffer));
            ssize_t n=read(fd,buffer,sizeof(buffer));
            //Logger::instance().log(Logger::DEBUG,"n="+to_string(n));
            if(n==-1){
                if(errno==EAGAIN||errno==EWOULDBLOCK) break;
                perror("read");
                del_fd(fd);
                return;
            }
            else if(n==0){
                del_fd(fd);
                return;
            }
            else{
                clients[fd].msg+=string(buffer,n);
            }
        }
        {
            unique_lock<mutex> lk(ct_mtx);
            clients[fd].last_active=time(0);
        }
        pool.enqueue([this,fd](){
            enqueue_out(fd);
        });
    }
    void handle_write(int fd){
        string& c_msg=clients[fd].msg;
        while(!c_msg.empty()){
            ssize_t n=write(fd,c_msg.data(),c_msg.size());
            if (n == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)  break;
                perror("write");
                clients.erase(fd);
                del_epoll(fd);
                Logger::instance().log(Logger::ERRNO, "客户端" + to_string(fd) + "异常断开");
                break;
            }
            else if (n == 0)
            {
                clients.erase(fd);
                del_epoll(fd);
                Logger::instance().log(Logger::INFO, "客户端" + to_string(fd) + "断开");
                break;
            }
            else
            {
                c_msg.erase(0,n);
            }
        }
        if(!c_msg.empty()) mod_epoll(fd,EPOLLIN|EPOLLOUT);
    }
public:
    Server(const ServerConfig& sCfg):PORT(sCfg.PORT),LEN(sCfg.LEN),HEART_BEAT(sCfg.HEART_BEAT),TIME_OUT(sCfg.TIME_OUT){};
    void setUpServer(){
        epfd=epoll_create1(0);
        vector<epoll_event> events(MAX_SIZE);
        set_Listen_fd();
        //set_Timer_fd();
        set_efd();
        listen(listen_fd,128);
        Logger::instance().log(Logger::INFO,"服务端已成功启动！");
        while(true){
            int nfds=epoll_wait(epfd,events.data(),MAX_SIZE,-1);
            if(nfds==-1){
                perror("epoll_wait");
                continue;
            }
            for(int i=0;i<nfds;i++){
                Logger::instance().log(Logger::DEBUG,"开始");
                int fd=events[i].data.fd;
                if(fd==listen_fd){
                    Logger::instance().log(Logger::DEBUG,"新连接");
                    handle_new_connect();
                    continue;
                }
                if(fd==efd){
                    Logger::instance().log(Logger::DEBUG,"处理任务");
                    uint64_t one;
                    while(read(efd,&one,sizeof(one))>0);
                    pool.enqueue([this](){
                        handle_task();
                    });
                    continue;
                }
                // if(fd==timer_fd){
                //     uint64_t one;
                //     while(read(timer_fd,&one,sizeof(one))>0);
                //     auto now=time(0);
                //     for(auto [fd,c]:clients){
                //         if(now-c.last_active>TIME_OUT) to_del.push_back(fd);
                //     }
                //     {
                //         unique_lock<mutex> lk(ct_mtx);
                //         for(auto fd:to_del){
                //             clients.erase(fd);
                //             del_epoll(fd);
                //             Logger::instance().log(Logger::INFO,"客户端"+to_string(fd)+"超时断开");
                //         }
                //     }
                //     to_del.clear();
                //     continue;
                // }
                if(events[i].events & EPOLLIN){
                    Logger::instance().log(Logger::DEBUG,"有读任务");
                    handle_read(fd);
                }
                if(events[i].events & EPOLLOUT){
                    Logger::instance().log(Logger::DEBUG,"有写任务");
                    handle_write(fd);
                }
                Logger::instance().log(Logger::DEBUG,"结束");
            }
        }
    }

};