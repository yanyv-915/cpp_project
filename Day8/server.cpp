#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/socket.h>
#include<cstring>
#include<iostream>
#include<algorithm>
#include <unordered_map>
#include<chrono>
#include"ThreadPool.h"
using namespace std;
vector<int>clients;
mutex c_mtx;
unordered_map<int,chrono::steady_clock::time_point>last_active;
unordered_map<int,int>timeout_check;

void broadcast(int send_fd,string msg){
    unique_lock<mutex> lock(c_mtx);
    for(int client_fd:clients){
        if(client_fd!=send_fd){
            string _msg="[客户端"+to_string(send_fd)+"]的消息："+msg;
            ssize_t n=write(client_fd,_msg.c_str(),_msg.size());
            if(n==-1){
                perror("write");
                close(client_fd);
                continue;
            }
        }
    }
}

void handle_client(int client_fd){
    char buffer[1024];
    while(true){
        ssize_t n=read(client_fd,buffer,sizeof(buffer));
        if(n>0){
            {
                unique_lock<mutex> lock(c_mtx);
                last_active[client_fd]=chrono::steady_clock::now();
                timeout_check[client_fd]=0;
            }
            buffer[n]='\0';
            if(strcmp(buffer,"exit")==0)    break;
            if(strcmp(buffer,"PING")==0){
                write(client_fd,"PONG",4);
                continue;
            }
            cout<<"["<<"客户端]"<<client_fd<<"的消息: "<<buffer<<endl;
            string msg=buffer;
            broadcast(client_fd,buffer);
        }
        else if(n==0){
            break;
        }
        else{
            perror("read");
            break;
        }
    }
    cout<<"客户端["<<client_fd<<"]已断开"<<endl;
    close(client_fd);
    {
        unique_lock<mutex> lock(c_mtx);
        clients.erase(remove(clients.begin(),clients.end(),client_fd),clients.end());
        last_active.erase(client_fd);
    }
}

void Heartbeat_checker(){
    while(true){
        this_thread::sleep_for(30s);
        auto now=chrono::steady_clock::now();
        unique_lock<mutex> lock(c_mtx);
        for(auto it=clients.begin();it!=clients.end();){
            int fd=*it;
            auto last_time=last_active[fd];
            if(chrono::duration_cast<chrono::seconds>(now-last_time).count()>30){
                timeout_check[fd]++;
                if(timeout_check[fd]>=3){
                    cout << "客户端[" << fd << "]超时未连接，已自动断开" << endl;
                    close(fd);
                    it = clients.erase(it);
                    last_active.erase(fd);
                    timeout_check.erase(fd);
                }
            }
            else{
                timeout_check[fd]=0;
                it++;
            }
        }
    }
}
int main(){
    ThreadPool pool(4);

    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        perror("socket");
        return -1;
    }
    sockaddr_in serv_addr{};
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(12345);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(listen_fd,(sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        perror("bind");
        close(listen_fd);
        return -1;
    }
    if(listen(listen_fd,4)==-1){
        perror("listen");
        close(listen_fd);
        return -1;
    }
    cout<<"服务器正在运行！"<<endl;
    thread checker(Heartbeat_checker);
    checker.detach();
    while(true){
        sockaddr_in client_addr;
        client_addr.sin_family=AF_INET;
        socklen_t client_len=sizeof(client_addr);
        int client_fd=accept(listen_fd,(sockaddr*)&client_addr,&client_len);
        if(client_fd==-1){
            if (errno == EINTR) continue;
            perror("accept");
            close(listen_fd);
            return -1;
        }
        cout<<"客户端["<<client_fd<<"]已连接！"<<endl;
        {
            unique_lock<mutex> lock(c_mtx);
            clients.push_back(client_fd);
            timeout_check[client_fd]=0;
        }
        pool.enqueue([client_fd](){
            handle_client(client_fd);
        });
    }
    close(listen_fd);
    return 0;
}