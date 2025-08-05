#include<cstring>
#include<iostream>
#include<arpa/inet.h>
#include<unistd.h>
using namespace std;
int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in se_addr{};
    se_addr.sin_family=AF_INET;
    se_addr.sin_port=htons(12345);
    se_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    socklen_t se_len=sizeof(se_addr);
    bind(listen_fd,(sockaddr*)&se_addr,se_len);
    listen(listen_fd,5);

    sockaddr_in client_addr{};
    socklen_t client_len=sizeof(client_addr);
    int conn_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,INET_ADDRSTRLEN);
    cout<<"客户端地址:"<<client_ip<<endl;
    close(conn_fd);
    close(listen_fd);
    return 0;
}
