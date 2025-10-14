#pragma once
#include<string>
#include<arpa/inet.h>
#include<cstring>
enum MsgType{
    To_one = 1,
    Broadcast=2,
    Heart_beat=3
};

struct MsgHeader{
    uint16_t MsgType;
    uint32_t length;
};

std::string packMsg(uint16_t type,const std::string& msg){
    MsgHeader header{};
    header.MsgType=htons(type);
    header.length=htonl(msg.size());
    std::string new_msg(msg.size()+sizeof(header),0);
    memcpy(new_msg.data(),&header,sizeof(header));
    memcpy(new_msg.data()+sizeof(header),msg.data(),msg.size());
    return new_msg;
}
