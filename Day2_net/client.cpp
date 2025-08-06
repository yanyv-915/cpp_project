#include<iostream>
#include<unistd.h>
#include<arpa/inet.h>
#include<cstring>
using namespace std;
int main(){
    //创建 socket        → socket()
    int client_fd=socket(AF_INET,SOCK_STREAM,0);

    //设置服务器地址结构   → sockaddr_in + inet_pton()
    sockaddr_in client_addr{};
    client_addr.sin_family=AF_INET;
    client_addr.sin_port=htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr);
    //发起连接请求        → connect()
    connect(client_fd,(sockaddr*)&client_addr,sizeof(client_addr));

    //通信 + 关闭连接     → write()/read()/close()
    const char* msg="Hello server!";
    write(client_fd,msg,strlen(msg));

    char buf[1024]={0};
    read(client_fd,buf,sizeof(buf));
    cout<<"From server:"<<buf<<endl;

    close(client_fd);
}