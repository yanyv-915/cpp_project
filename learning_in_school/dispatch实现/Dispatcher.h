#pragma once
#include"Logger.h"
#include"Task.h"
#include"MsgHeader.h"
#include<functional>
#include<unordered_map>
class Dispatcher{
public:
    using Handler = std::function<void(Task&)>;

    void register_handle(int msgType,Handler handler){
        std::lock_guard<std::mutex> lk(mtx);
        handlers[msgType]=handler;
        Logger::instance().log(Logger::INFO,"已注册消息类型:"+std::to_string(msgType));
    }
    void dispatch(Task& t,int msgType){
        std::lock_guard<std::mutex> lk(mtx);
        auto it=handlers.find(msgType);
        if(it!=handlers.end()){
            it->second(t);
        }
        else{
            Logger::instance().log(Logger::WARNING,"未注册消息类型:"+std::to_string(msgType));
        }
    }
private:
    std::unordered_map<int,Handler>handlers;
    std::mutex mtx;
};