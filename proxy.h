#ifndef WEB_H
#define WEB_H

#include <winsock2.h>
#include <string>
#include <vector>

#pragma comment(lib,"Ws2_32.lib")

using namespace std;

//Http 重要头部数据
struct HttpHeader{
    char method[4]; // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
    char url[1024]; // 请求的 url
    char host[1024]; // 目标主机
    char cookie[1024 * 10]; //cookie
    HttpHeader(){
        ZeroMemory(this,sizeof(HttpHeader));
    }
};
    
//传递给线程所需参数
struct ProxyParam{
    string banUrlPath = "./banURL.txt";
    string banIPPath = "./banIP.txt";
    BOOL fished;
    SOCKET clientSocket;
    SOCKET serverSocket;
};

//代理服务器类
class Proxy{

    private:
        const int ProxyPort = 8080;             //代理服务器端口
        SOCKET ProxyServer;                     //代理服务器套接字
        sockaddr_in ProxyServerAddr;            //代理服务器地址信息
        string banUrlPath = "./banURL.txt";     //存储被禁止的URL对应txt文件路径
        string banIPPath = "./banIP.txt";       //存储被禁止的IP对应txt文件路径

    public:


        //************************************
        // Method: InitSocket
        // FullName: InitSocket
        // Access: public 
        // Returns: BOOL
        // Qualifier: 初始化套接字
        //************************************
        BOOL InitSocket();

        //************************************
        // Method: GetPort
        // FullName: GetProxyPort
        // Access: public 
        // Returns: int
        // Qualifier: 获取代理端口号
        //************************************
        int GetPort();

        //************************************
        // Method: Start
        // FullName: Start
        // Access: public 
        // Returns: BOOL
        // Qualifier: 启动代理服务器
        //************************************
        BOOL Start(vector<string> bansURL, vector<string> bansIP, BOOL fished);
};

#endif