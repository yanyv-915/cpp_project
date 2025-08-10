#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<iostream>
#include<cstring>
#include<thread>
using namespace std;

void handle_client(int conn_fd,sockaddr_in client_addr){
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,INET_ADDRSTRLEN);
    cout<<"客户端："<<client_ip<<endl;
    char buffer[1024];
    while(true){
        memset(buffer,0,sizeof(buffer));
        ssize_t n=read(conn_fd,buffer,sizeof(buffer));
        if(n<=0) break;
        cout<<"[来自"<<client_ip<<"]的   "<<buffer<<endl;
        if(strcmp(buffer,"exit")==0) break;
        string reply="[echo]";
        reply+=buffer;
        write(conn_fd,reply.c_str(),reply.size());
    }
    cout<<"连接关闭来自["<<client_ip<<"]的服务端"<<endl;
    return;
}
int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in serv_addr{};
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(12345);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    bind(listen_fd,(sockaddr*)&serv_addr,sizeof(serv_addr));
    listen(listen_fd,5);
    cout<<"服务器已启动，等待客户端连接。。。"<<endl;
    while(true){
        sockaddr_in client_addr;
        client_addr.sin_family=AF_INET;
        socklen_t client_len=sizeof(client_addr);
        int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);
        thread t(handle_client,conn_fd,client_addr);
        t.detach();
    }
    close(listen_fd);
    return 0;
}