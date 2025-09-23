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
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<ctime>
#include<atomic>
#include<thread>
#include<vector>
#include<string>
#include<atomic>
#define PORT 12345
#define MAX_BUF 4096
#define MAX_EVENT 1024
#define SERVER_IP "127.0.0.1"
