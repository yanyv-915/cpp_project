#pragma once
#include<ctime>
#include<iostream>
#include<string>
#include<mutex>
#include<fstream>
using namespace std;
class Logger{
public:
    enum LogLevel{
        INFO,
        WARNING,
        ERRNO,
        DEBUG
    };
    static Logger& instance(){
        static Logger logger;
        return logger;
    }
    void log(LogLevel level,const string& msg){
        lock_guard<mutex> lk(mtx);
        string out=getTime()+LevelToString(level)+":"+msg;
        cout<<out<<endl;
        if(ofs.is_open()){
            ofs<<out<<endl;
        }
    }
    void setFile(const string& filename){
        lock_guard<mutex> lk(mtx);
        ofs.open(filename,ios::app);
    }

private:
    mutex mtx;
    ofstream ofs;
    Logger(){};
    ~Logger(){
        if(ofs.is_open()) ofs.close();
    }
    string getTime(){
        time_t now=time(0);
        char buf[32];
        strftime(buf,sizeof(buf),"%Y-%M-%D %H:%m:%s ",localtime(&now));
        return string(buf);
    }
    string LevelToString(LogLevel level){
        switch (level){
            case INFO: return "[INFO]";
            case WARNING: return "[WARNING]";
            case ERRNO: return "[ERRNO]";
            case DEBUG: return "[DEBUG]";
        };
        return "[UNKNOWN]";
    }
};