#include<iostream>
#include<fstream>
#include<mutex>
#include<string>
#include<ctime>
using namespace std;
class Logger{
public:
    enum  Loglevel{
        INFO,
        WARING,
        ERROR,
        DEBUG
    };
    static Logger& instance(){
        static Logger Logger;
        return Logger;
    }
    void log(Loglevel level,const string& msg){
        lock_guard<mutex> lk(mtx);
        string prefix=getTime()+LevelToStr(level)+":"+msg;
        cout<<prefix<<endl;
        if(ofs.is_open()){
            ofs<<prefix<<endl;
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
        time_t now=time(nullptr);
        char buffer[32];
        strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S ",localtime(&now));
        return string(buffer);
    }
    string LevelToStr(Loglevel level){
        switch(level)
        {
            case INFO: return "[INFO]";
            case WARING: return "[WARNIG]";
            case ERROR: return "[ERROR]";
            case DEBUG: return "[DEBUG]";
        }
        return "[UNKNOWN]";
    }



};