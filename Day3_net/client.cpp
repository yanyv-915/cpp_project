#include<arpa/inet.h>
#include<unistd.h>
#include<iostream>
#include<cstring>
using namespace std;
int main(){
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in c_addr{};
    c_addr.sin_family=AF_INET;
    c_addr.sin_port=htons(12345);
    inet_pton(AF_INET,"127.0.0.1",&c_addr.sin_addr.s_addr);
    connect(sock_fd,(sockaddr*)&c_addr,sizeof(c_addr));
    cout<<"连接成功！"<<endl;
    close(sock_fd);
    return 0;
}