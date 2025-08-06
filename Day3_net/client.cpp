#include<arpa/inet.h>
#include<unistd.h>
#include<iostream>
#include<cstring>
using namespace std;
int main(){
    //生成客户端端口
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);

    //设定客户端端口地址
    sockaddr_in c_addr{};
    c_addr.sin_family=AF_INET;
    c_addr.sin_port=htons(12345);

    //将字符串形式的IP地址转化为二进制格式的可绑定地址格式
    inet_pton(AF_INET,"127.0.0.1",&c_addr.sin_addr.s_addr);

    //连接客户端端口文件
    connect(sock_fd,(sockaddr*)&c_addr,sizeof(c_addr));
    cout<<"连接成功！"<<endl;

    //关闭端口
    close(sock_fd);
    return 0;
}