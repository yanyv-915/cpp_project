#include"MyHeader.h"
using namespace std;
mutex cout_mtx;
atomic<bool>running(true);

int send_msg(int server_fd,const string& msg){
    uint32_t len=htonl(msg.size());
    vector<char> data(msg.size()+sizeof(len));
    memcpy(data.data(),&len,sizeof(len));
    memcpy(data.data()+sizeof(len),msg.data(),msg.size());
    return send(server_fd,data.data(),data.size(),0);
}
void hanle_read(int server_fd){
    vector<char> buffer;
    char tmp[MAX_BUF];
    while(running){
        int n=recv(server_fd,tmp,sizeof(tmp),0);
        if(n<=0){
            perror("write");
            lock_guard<mutex> lk(cout_mtx);
            cout<<"服务器已关闭"<<endl;
            running=false;
            break;
        }
        buffer.insert(buffer.end(),tmp,tmp+n);
        while(true){
            if(buffer.size()<4) break;
            uint32_t msg_len;
            memcpy(&msg_len,buffer.data(),4);
            msg_len=ntohl(msg_len);
            if(buffer.size()<4+msg_len) break;
            string message=string(buffer.begin()+4,buffer.begin()+4+msg_len);
            lock_guard<mutex> lk(cout_mtx);
            cout<<message<<endl;
            buffer.erase(buffer.begin(),buffer.begin()+4+msg_len);
        }
    }
}
void handle_beat(int server_fd){
    while(running){
        send_msg(server_fd,"PING");
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
        int n=send_msg(server_fd,msg);
        if(n==-1){
            perror("write");
            lock_guard<mutex> lk(cout_mtx);
            cout<<"服务器已关闭"<<endl;
            running=false;
            break;
        }
        if(msg=="exit"){
            lock_guard<mutex> lk(cout_mtx);
            cout<<"客户端主动断开连接"<<endl;
            running=false;
        }
    }
    close(server_fd);
    return 0;
}