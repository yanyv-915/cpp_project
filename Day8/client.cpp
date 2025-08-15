#include<sys/socket.h>
#include<arpa/inet.h>
#include<iostream>
#include<cstring>
#include<unistd.h>
#include<thread>
#include<chrono>
using namespace std;
void server_checker(int server_fd){
    while (true){
        this_thread::sleep_for(10s);
        ssize_t n= write(server_fd, "PING", 4);
        if(n==-1){
            break;
        }
    }
}
void read_server(int server_fd){
    char buffer[1024];
    while(true){
        ssize_t n=read(server_fd,buffer,sizeof(buffer));
        if(n>0){
            buffer[n]='\0';
            if(strcmp(buffer,"PONG")==0) continue;
            cout<<buffer<<endl;
        }
        else if(n==0){
            cout<<"服务端已关闭"<<endl;
            break;
        }
        else{
            perror("read");
            break;
        }
    }
}
int main(){
    int server_fd=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(12345);
    inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr);

    int client_fd=connect(server_fd,(sockaddr*)&server_addr,sizeof(server_addr));
    if(client_fd==-1){
        perror("connect");
        return 1;
    }
    cout<<"服务端连接成功！"<<endl;
    thread reader(read_server,server_fd);
    reader.detach();
    thread checker(server_checker,server_fd);
    checker.detach();
    string msg;
    while(true){
        getline(cin,msg);
        if(msg=="exit"){
            write(server_fd,"客户端退出",sizeof("客户端退出"));
            break;
        }
        ssize_t n=write(server_fd,msg.c_str(),msg.size());
        if(n==-1){
            perror("write");
            close(client_fd);
            return -1;
        }
    }
    close(client_fd);
    return 0;
}