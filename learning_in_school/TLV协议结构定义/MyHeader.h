#pragma once
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<sys/timerfd.h>
#include<iostream>
#include<string>
#include<unordered_map>
#include<vector>
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<ctime>
#include<thread>
#include<atomic>
#include"Protocol.h"
#include"Logger.h"
#include"config.h"
#include"ThreadPool.h"
#define MAX_BUF 4096
#define MAX_SIZE 1024
#define CHECK_ERR(expr,msg)do{\
    if((expr)==-1){\
        perror(msg);\
        exit(EXIT_FAILURE);\
    }\
}while(0)
using namespace std;

