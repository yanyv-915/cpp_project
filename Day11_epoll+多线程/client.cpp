#include<iostream>
#include<unistd.h>
#include<netinet/in.h>
#include<cstring>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<thread>
#include<atomic>
using namespace std;
#define SERVER_IP "127.0.0.1"
atomic<bool>running(true);
void hanle_read(int server_fd){
    char buffer[1024];
    while(running){
        memset(buffer,0,sizeof((buffer)));
        ssize_t count=read(server_fd,buffer,sizeof(buffer));
        if(count==-1){
            perror("read");
            cout<<"线程接收失败"<<endl;
            running=false;
            return;
        }
        else if(count==0){
            cout<<"服务端已关闭"<<endl;
            running=false;
        }
        else{
            if (strcmp(buffer, "PONG") == 0)  continue;
            buffer[count]='\0';
            cout << buffer << endl;
        }
        
    }
}
void handle_beat(int server_fd){
    while(running){
        ssize_t count=write(server_fd,"PING",4);
        if(count==-1){
            cout<<"服务端连接可能断开"<<endl;
            running=false;
            return;
        }
        this_thread::sleep_for(10s);
    }
}
int main(){
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd==-1){
        perror("socket");
        return -1;
    }
    sockaddr_in server_addr{};
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(12345);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);

    if(connect(server_fd,(sockaddr*)&server_addr,sizeof(server_addr))==-1){
        perror("conncet");
        return -1;
    }
    cout<<"服务器已成功连接"<<endl;
    thread reader(hanle_read,server_fd);
    thread heart(handle_beat,server_fd);
    reader.detach();
    heart.detach();
    while(running){
        string msg;
        getline(cin,msg);
        int n=write(server_fd,msg.c_str(),msg.size());
        if(n==-1){
            perror("write");
            cout<<"服务器已关闭"<<endl;
            running=false;
            break;
        }
        if(msg=="exit"){
            cout<<"客户端主动断开连接"<<endl;
            running=false;
        }
    }
    close(server_fd);
    return 0;
}