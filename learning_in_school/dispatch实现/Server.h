#pragma once
#include"Dispatcher.h"
#include<arpa/inet.h>
#include<cstring>
using namespace std;

class Server{
private:
    Dispatcher dispatcher;
    void handle_ping(Task& task) {
        Logger::instance().log(Logger::DEBUG, "收到 PING -> 回复 PONG");
    }
    void handle_pong(Task& task) {
        Logger::instance().log(Logger::DEBUG, "收到 PONG 响应");
    }
    void handle_chat(Task& task) {
        Logger::instance().log(Logger::INFO, "收到 CHAT 消息: " + task.msg.substr(sizeof(MsgHeader)));
    }
public:
    void init_handlers(){
        dispatcher.register_handle(Msg_PING,[this](Task& task){
            handle_ping(task);
        });
        dispatcher.register_handle(Msg_PONG,[this](Task& task){
            handle_pong(task);
        });
        dispatcher.register_handle(Msg_CHAT,[this](Task& task){
            handle_chat(task);
        });
    }
    void handle_task(Task& task){
        MsgHeader header{};
        memcpy(&header,task.msg.data(),sizeof(header));
        int type = ntohs(header.MsgType);
        dispatcher.dispatch(task,type);
    }
};