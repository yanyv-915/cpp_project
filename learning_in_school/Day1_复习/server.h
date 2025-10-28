#pragma once
#include"MyHeader.h"
#include"config.h"
#include"ThreadPool.h"
#include"Logger.h"
#define MAX_BUF 4096
#define MAX_SIZE 1024
using namespace std;
class Server{
private:
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
    queue<task_o>tasks_o;
    queue<task_b>tasks_b;
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
    string process_packet(int fd,const string& msg){
        string m="客户端"+to_string(fd)+":"+msg;
        uint32_t len=m.size();
        uint32_t net_len=htonl(len);
        string packet;
        packet.resize(LEN+len);
        memcpy(packet.data(),&net_len,LEN);
        memcpy(packet.data()+LEN,m.data(),len);
        return packet;
    }
    void server_out(int fd,const string& msg){
        Logger::instance().log(Logger::INFO,"客户端"+to_string(fd)+":"+msg);
    }
    void enqueue_o(int recieve_fd,const string& msg,int send_fd=0){
        lock_guard<mutex> lk(ct_mtx);
        tasks_o.push({send_fd,recieve_fd,msg});
        uint64_t one=1;
        write(efd,&one,sizeof(one));
    }
    void enqueue_b(int send_fd,const string& msg){
        lock_guard<mutex> lk(ct_mtx);
        tasks_b.push({send_fd,msg});
        uint64_t one=1;
        write(efd,&one,sizeof(one));
    }
    void to_one(){
        lock_guard<mutex> lk(ct_mtx);
        while(!tasks_o.empty()){
            auto t=move(tasks_o.front());
            tasks_o.pop();
            if(clients.find(t.recieve_fd)==clients.end()) continue;
            string& msg=clients[t.recieve_fd].msg;
            msg.append(t.msg);
            if(msg.size()>LEN){
                uint32_t len;
                memcpy(&len,msg.data(),LEN);
                len=ntohl(len);
                if(msg.size()<len+LEN) continue;
                ssize_t n=write(t.recieve_fd,msg.data(),LEN+len);
                if(n>0){
                    msg.erase(0,n);
                    if(!msg.empty()){
                        mod_epoll(t.recieve_fd,EPOLLIN|EPOLLOUT);
                    }
                }
                else{
                    if(n==-1&&(errno==EWOULDBLOCK||errno==EAGAIN)){
                        mod_epoll(t.recieve_fd,EPOLLIN|EPOLLOUT);
                        continue;
                    }
                    else{
                        perror("write");
                        Logger::instance().log(Logger::ERRNO,"客户端"+to_string(t.recieve_fd)+"异常断开");
                        del_fd(t.recieve_fd);
                        continue;
                    }             
                }

            }
        }
    }
    void broadcast()
    {
        lock_guard<mutex> lk(ct_mtx);
        while (!tasks_b.empty())
        {
            auto t = move(tasks_b.front());
            tasks_b.pop();
            if(clients.find(t.send_fd)==clients.end()) continue;
            for (auto &[fd, c] : clients)
            {
                if (fd == t.send_fd)
                    continue;

                c.msg.append(t.msg);
                if (c.msg.size() > LEN)
                {
                    uint32_t len;
                    memcpy(&len, c.msg.data(), LEN);
                    len = ntohl(len);
                    if (c.msg.size() < len + LEN)
                        continue;
                    ssize_t n = write(fd, c.msg.data(), LEN + len);
                    if (n > 0)
                    {
                        c.msg.erase(0, n);
                        if (!c.msg.empty())
                        {
                            mod_epoll(fd, EPOLLIN | EPOLLOUT);
                        }
                    }
                    else
                    {
                        if (n == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
                        {
                            mod_epoll(fd, EPOLLIN | EPOLLOUT);
                            continue;
                        }
                        else
                        {
                            perror("write");
                            Logger::instance().log(Logger::ERRNO, "客户端" + to_string(fd) + "异常断开");
                            del_fd(fd);
                            continue;
                        }
                    }
                }
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
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
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
        unique_lock<mutex> lk(ct_mtx);
        if(clients.find(fd)==clients.end()) return;
        char buf[MAX_BUF];
        uint32_t len;
        string real_msg;
        while (true)
        {
            memset(buf,0,sizeof(buf));
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("read");
                Logger::instance().log(Logger::ERRNO, "客户端" + to_string(fd) + "异常断开");
                lk.unlock();
                del_fd(fd);
                break;
            }
            else if (n == 0)
            {
                Logger::instance().log(Logger::INFO, "客户端" + to_string(fd) + "断开连接");
                lk.unlock();
                del_fd(fd);
                break;
            }
            else
            {
                clients[fd].last_active=time(0);
                clients[fd].msg.append(buf,n);
                while(clients[fd].msg.size()>LEN)
                {
                    memcpy(&len, clients[fd].msg.data(), LEN);
                    len = ntohl(len);
                    if(clients[fd].msg.size()<len+LEN) break;
                    real_msg.resize(len);
                    memcpy(real_msg.data(),clients[fd].msg.data()+LEN,len);
                    lk.unlock();
                    pool.enqueue([this,fd,real_msg](){
                        if(real_msg=="PING"){
                            //have write
                            enqueue_o(fd,process_packet(fd,"PONG"));
                        }
                        else{
                            //not write
                            server_out(fd,real_msg);
                            //have write
                            enqueue_b(fd,process_packet(fd,real_msg));
                            
                        }
                    });
                    lk.lock();
                    if(clients.find(fd)==clients.end()) return;
                    clients[fd].msg.erase(0,LEN+len);
                }
            }
        }
    }
    void handle_write(int fd){
        unique_lock<mutex> lk(ct_mtx);
        if(clients.find(fd)==clients.end()) return;
        auto& c=clients[fd];
        while (c.msg.size() > LEN)
        {
            uint32_t len;
            memcpy(&len, c.msg.data(), LEN);
            len = ntohl(len);
            if (c.msg.size() < len + LEN)
                break;
            ssize_t n = write(fd, c.msg.data(), LEN + len);
            if (n > 0)
            {
                c.msg.erase(0, n);
                if (!c.msg.empty())
                {
                    mod_epoll(fd, EPOLLIN | EPOLLOUT);
                }
            }
            else
            {
                if (n == -1 && errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    mod_epoll(fd, EPOLLIN | EPOLLOUT);
                    break;
                }
                else
                {
                    perror("write");
                    Logger::instance().log(Logger::ERRNO, "客户端" + to_string(fd) + "异常断开");
                    lk.unlock();
                    del_fd(fd);
                    break;
                }
            }
        }
    }
public:
    Server(const ServerConfig& sCfg):PORT(sCfg.PORT),LEN(sCfg.LEN),HEART_BEAT(sCfg.HEART_BEAT),TIME_OUT(sCfg.TIME_OUT){};
    void setUpServer(){
        epfd=epoll_create1(0);
        vector<epoll_event> events(MAX_SIZE);
        set_Listen_fd();
        set_Timer_fd();
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
                //Logger::instance().log(Logger::DEBUG,"循环开始");
                int fd=events[i].data.fd;
                if(fd==listen_fd){
                    //Logger::instance().log(Logger::DEBUG,"新连接");
                    handle_new_connect();
                    continue;
                }
                if(fd==efd){
                    //Logger::instance().log(Logger::DEBUG,"新事件");
                    uint64_t one;
                    while(read(efd,&one,sizeof(one))>0);
                    pool.enqueue([this](){
                        to_one();
                    });
                    pool.enqueue([this](){
                        broadcast();
                    });
                    continue;
                }
                if(fd==timer_fd){
                    //Logger::instance().log(Logger::DEBUG,"心跳监听");
                    uint64_t one;
                    while(read(timer_fd,&one,sizeof(one))>0);
                    auto now=time(0);
                    for(auto [fd,c]:clients){
                        if(now-c.last_active>TIME_OUT) to_del.push_back(fd);
                    }
                    {
                        unique_lock<mutex> lk(ct_mtx);
                        for(auto fd:to_del){
                            clients.erase(fd);
                            del_epoll(fd);
                            Logger::instance().log(Logger::INFO,"客户端"+to_string(fd)+"超时断开");
                        }
                    }
                    to_del.clear();
                    continue;
                }
                if(events[i].events & EPOLLIN){
                    //Logger::instance().log(Logger::DEBUG,"读事件");
                    handle_read(fd);
                }
                if(events[i].events & EPOLLOUT){
                    //Logger::instance().log(Logger::DEBUG,"有写事件");
                    unique_lock<mutex> lk(ct_mtx);
                    handle_write(fd);
                }
                //Logger::instance().log(Logger::DEBUG,"循环结束");
            }
        }
    }

};