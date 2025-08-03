#include<iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<cstring>
#include<arpa/inet.h>
#include<sys/types.h>
using namespace std;

int main(){
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("socket");
        return 1;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(12345);
    if(bind(server_fd,(sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("bind绑定失败");
        return 1; 
    }

    if(listen(server_fd,5)<0){
        perror("listen监听失败");
        return 1;
    }

    cout<<"服务器启动，等待连接。。。。。。\n";


    sockaddr_in client_addr{};
    socklen_t client_len=sizeof(client_addr);
    int client_fd=accept(server_fd,(sockaddr*)&client_addr,&client_len);
    if(client_fd<0){
        perror("accept接受失败");
        return 1;
    }

    char buffer[1024];
    while(true){
        memset(buffer,0,sizeof(buffer));
        int byte_read=recv(client_fd,buffer,sizeof(buffer),0);
        if(byte_read<=0){
            perror("用户已断开连接");
            break;
        }
        cout<<"收到\n";
    send(client_fd,buffer,byte_read,0);
    }
    close(client_fd);
    close(server_fd);
    return 0;
}
