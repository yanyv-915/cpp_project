#pragma once
#include "MyHeader.h"
using namespace std;
class Client
{
private:
    int PORT;
    string SERVER_IP;
    int LEN;
    int server_fd;
    atomic<bool> running;
    ThreadPool pool;
    void handle_read()
    {
        char buf[MAX_BUF];
        string msg="";
        while(running){
            memset(buf,0,sizeof(buf));
            ssize_t n=read(server_fd,buf,sizeof(buf));
            if(n==-1||n==0){
                Logger::instance().log(Logger::ERRNO,"服务端断开连接");
                running=false;
                if(n==-1) perror("write");
                running=false;
                break;
            }
            else{
                msg+=string(buf,n);
            }
            MsgHeader header{};
            while(msg.size()>sizeof(header)){
                memset(&header,0,sizeof(header));
                memcpy(&header, msg.data(), sizeof(header));
                uint16_t type = ntohs(header.MsgType);
                uint32_t len = ntohl(header.length);
                std::string ans;
                if (msg.size() < len + sizeof(header)) break;
                ans.resize(len);
                memcpy(ans.data(), msg.data() + sizeof(header), len);
                msg.erase(0,sizeof(header) + len);
                if(type==Heart_beat) continue;
                Logger::instance().log(Logger::INFO,ans);
            }
        }
    }
    void handle_write()
    {
        string msg;
        while(running){
            getline(cin,msg);
            string p_msg=packMsg(Broadcast,msg);
            ssize_t n=write(server_fd,p_msg.data(),p_msg.size());
            if(n==-1||n==0){
                perror("write");
                Logger::instance().log(Logger::ERRNO,"服务端断开连接");
                running=false;
                break;
            }
        }
    }
    void heart_beat()
    {
        while(running){
            //Logger::instance().log(Logger::DEBUG,"心跳检测");
            string ping_msg=packMsg(Heart_beat,"PING");
            ssize_t n=write(server_fd,ping_msg.data(),ping_msg.size());
            if(n==-1||n==0){
                perror("write");
                Logger::instance().log(Logger::ERRNO,"服务端断开连接");
                running=false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    void work(){
        pool.enqueue([this](){
            handle_read();
        });
        pool.enqueue([this](){
            handle_write();
        });
        pool.enqueue([this](){
            heart_beat();
        });
    }
public:
    Client(const ClientConfig &cCfg) : PORT(cCfg.PORT), SERVER_IP(cCfg.SERVER_IP), LEN(cCfg.LEN), running(true) {};
    void run(){
        server_fd=socket(AF_INET,SOCK_STREAM,0);
        sockaddr_in server_addr{};
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(PORT);
        inet_pton(AF_INET,SERVER_IP.data(),&server_addr.sin_addr);
        connect(server_fd,(sockaddr*)&server_addr,sizeof(server_addr));
        Logger::instance().log(Logger::INFO,"成功与服务端连接");
        work();
        if(!running){
            close(server_fd);
        }
    }
};