// -*- coding: utf-8 -*-
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <string>
#include "proxy.h"

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

//由于新的连接都使用新线程进行处理，对线程的频繁的创建和销毁特别浪费资源
//可以使用线程池技术提高服务器效率
//const int ProxyThreadMaxNum = 20;
//HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
//DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};

int main(int argc, _TCHAR* argv[]){
    printf("Initialize the server...\n");
    Proxy *proxy = new Proxy;   //实例化代理服务器
    Sleep(1000);
    printf("Initialize done...\n");
    if(!proxy->InitSocket()){
        printf("Description Failed to initialize the socket\n");
        return -1;
    }

    //配置禁用URL与IP
    vector<string> bansURL = {""};
    vector<string> bansIP = {""};
    //启动代理服务器
    if(!proxy->Start(bansURL, bansIP, false)){
        printf("The proxy server is faulty");
    }
    return 0;
}