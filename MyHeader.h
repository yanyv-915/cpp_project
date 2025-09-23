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
#include<string>
#include<unordered_map>
#include<vector>
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<ctime>
#include<thread>
#include<atomic>
#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define MAX_BUF 4096
#define MAX_SIZE 1024
