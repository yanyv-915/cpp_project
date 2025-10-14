#pragma once
#include"json.hpp"
#include<iostream>
#include<fstream>
#include<string>
using json = nlohmann::json;
struct ServerConfig {
    int PORT;
    int HEART_BEAT;
    int TIME_OUT;
    int LEN;
};
struct ClientConfig{
    int PORT;
    std::string SERVER_IP;
    int LEN;
};
struct AppConfig {
    ServerConfig server;
    ClientConfig client;
};

class Config{
private:
    json j;
public:
    AppConfig cfg;
    Config(const std::string& filename) {
        std::ifstream in(filename);
        if(!in.is_open()){
            throw std::runtime_error("无法解析配置文件:"+filename);
        }
        in>>j;
        cfg.server.PORT=j["server"].value("PORT",12345);
        cfg.server.HEART_BEAT=j["server"].value("HEART_BEAT",8);
        cfg.server.TIME_OUT=j["server"].value("TIME_OUT",30);
        cfg.server.LEN=j["server"].value("LEN",4);
        cfg.client.PORT=j["client"].value("PORT",12345);
        cfg.client.SERVER_IP=j["client"].value("SERVER_IP","127.0.0.1");
        cfg.client.LEN=j["client"].value("LEN",4);
    }
};