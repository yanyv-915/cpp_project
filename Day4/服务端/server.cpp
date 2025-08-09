#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<cstring>
#include<iostream>
#include<unistd.h>//包含read
using namespace std;
int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in servaddr{};
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(12345);
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    bind(listen_fd,(sockaddr*)&servaddr,sizeof(servaddr));
    listen(listen_fd,5);
    cout<<"服务器已启动，等待连接..."<<endl;
    sockaddr_in clientaddr;
    socklen_t client_len=sizeof(clientaddr);
    int conn_fd=accept(listen_fd,(sockaddr*)&clientaddr,&client_len);
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET,&clientaddr.sin_addr,client_ip,INET_ADDRSTRLEN);
    cout<<"客户端连接自："<<client_ip<<endl;
    char buf[1024];
    while(true){
        memset(buf,0,sizeof(buf));
        ssize_t n=read(conn_fd,buf,sizeof(buf));
        if(n<=0) break;
        cout<<"收到"<<buf<<endl;
        if(strcmp(buf,"exit")==0) break;
        write(conn_fd,buf,strlen(buf));
    }
    close(conn_fd);
    close(listen_fd);
    return 0;
}
