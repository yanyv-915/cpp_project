#include"Server.h"
using namespace std;
string make_tlv(int msgType,const string& body){
    MsgHeader header{};
    header.MsgLen=htonl(body.length());
    header.MsgType=htons(msgType);
    string msg;
    msg.resize(sizeof(header)+body.length());
    memcpy(msg.data(),&header,sizeof(header));
    memcpy(msg.data()+sizeof(header),body.data(),body.length());
    return msg;
}
int main(){
    Server server;
    server.init_handlers();

    // 模拟 3 条消息
    Task t1{1, make_tlv(Msg_PING, "")};
    Task t2{2, make_tlv(Msg_CHAT, "Hello everyone!")};
    Task t3{3, make_tlv(999, "未知消息类型")};

    server.handle_task(t1);
    server.handle_task(t2);
    server.handle_task(t3);

    return 0;
}