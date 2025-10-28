#pragma once
#include<string>
#include<mutex>
#include<iostream>
class Logger{
public:
    enum LoggerLevel{
        INFO,
        WARNING,
        ERRNO,
        DEBUG
    };
    static Logger& instance(){
        static Logger logger;
        return logger;
    }
    void log(LoggerLevel level,const std::string& msg){
        std::lock_guard<std::mutex> lk(mtx);
        std::cout<<LevelToStr(level)+msg<<std::endl;
        return;
    }

private:
    std::mutex mtx;
    std::string LevelToStr(LoggerLevel& level){
        switch (level){
            case INFO:return "[INFO]:";
            case WARNING:return "[WARNING]:";
            case ERRNO:return "[ERRNO]:";
            case DEBUG:return "[DEBUG]:";
            default:break;
        }
        return "[UNKNOWN]:";
    }
};