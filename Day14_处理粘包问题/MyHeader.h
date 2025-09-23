#pragma
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<sys/timerfd.h>
#include<iostream>
#include<cstring>
#include<unordered_map>
#include<vector>
#include<fcntl.h>
#include<stdlib.h>
#include<thread>
#include<mutex>
#include<atomic>
#define PORT 12345
#define MAX_BUF 4096
#define MAX_EVENT 1024
#define SERVER_IP "127.0.0.1"
#define TIMEOUT 30
#define CHECK_ERR(exp,msg)\
    do{\
        if((exp)==-1){\
            perror(msg);\
            exit(EXIT_FAILURE);\
        }\
    }\
    while(0)
#define HEARTBEAT_INTERVAL 10
