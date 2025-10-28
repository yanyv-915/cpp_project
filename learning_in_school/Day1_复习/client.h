#pragma once
#include "MyHeader.h"
#include "config.h"
#include "Logger.h"
#include"ThreadPool.h"
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
    string process(const string& msg){
        uint32_t len = msg.size();
        uint32_t net_len=htonl(len);
        string packet;
        packet.resize(LEN + len);
        memcpy(packet.data(), &net_len, LEN);
        memcpy(packet.data() + LEN, msg.data(), msg.size());
        return packet;
    }
    void handle_read()
    {
        char buf[MAX_BUF];
        string msg="";
        uint32_t len;
        string real_msg;
        while (running)
        {
            memset(buf,0,sizeof(buf));
            ssize_t n = read(server_fd, buf, sizeof(buf));
            if (n == -1)
            {
                running = false;
                perror("read");
                Logger::instance().log(Logger::ERRNO, "服务端异常断开");
                break;
            }
            else if (n == 0)
            {
                running = false;
                Logger::instance().log(Logger::WARNING, "服务端断开");
                break;
            }
            else
            {
                msg.append(buf,n);
                while(msg.size()>LEN){
                    memcpy(&len,msg.data(),LEN);
                    len=ntohl(len);
                    if(msg.size()<len+LEN) break;
                    real_msg.resize(len);
                    memcpy(real_msg.data(),msg.data()+LEN,len);
                    msg.erase(0,len+LEN);
                    if (real_msg.size() >= 4 &&real_msg.compare(real_msg.size() - 4, 4, "PONG") == 0) continue;
                    Logger::instance().log(Logger::INFO,real_msg);
                }
            }
        }
        return;
    }
    void handle_write()
    {
        string msg;
        while (running)
        {
            getline(cin, msg);
            string packet = process(msg);
            ssize_t n = write(server_fd, packet.data(), packet.size());
            if (n == -1 || n == 0)
            {
                running = false;
                Logger::instance().log(Logger::ERRNO, "服务端异常断开");
                if (n == -1)
                    perror("write");
                break;
            }
        }
    }
    void heart_beat()
    {
        string m=process("PING");
        while (running)
        {
            ssize_t n = write(server_fd, m.data(), m.size());
            if (n == -1 || n == 0)
            {
                running = false;
                Logger::instance().log(Logger::ERRNO, "服务端异常断开");
                if (n == -1)
                    perror("read");
                break;
            }
            this_thread::sleep_for(5s);
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
    }
};