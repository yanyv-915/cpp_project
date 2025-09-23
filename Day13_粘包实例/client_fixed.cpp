#include"MyHeader.h"
using namespace std;
void sendMsg(int server_fd,const string& msg){
    uint32_t len=htonl(msg.size());
    send(server_fd,&len,sizeof(len),0);
    send(server_fd,msg.data(),msg.size(),0);
}
int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    connect(sock, (sockaddr*)&serv, sizeof(serv));
    sendMsg(sock, "Hello");
    sendMsg(sock, "World");
    sendMsg(sock, "!!!");

    close(sock);
    return 0;
}