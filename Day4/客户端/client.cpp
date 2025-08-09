#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<iostream>
using namespace std;
int main(){
    int sock=socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in serv_addr{};
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(12345);
    inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr);

    connect(sock,(sockaddr*)&serv_addr,sizeof(serv_addr));

    char buf[1024];
    while(true){
        cout<<"输出消息："<<endl;
        cin.getline(buf,sizeof(buf));
        write(sock,buf,strlen(buf));
        if(strcmp(buf,"exit")==0) break;
        memset(buf,0,sizeof(buf));
        read(sock,buf,sizeof(buf));
        cout<<"服务器返回："<<buf<<endl;

    }
    close(sock);
    return 0;
}