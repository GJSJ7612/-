#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <string.h>
#include <ws2tcpip.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <zlib.h>
#include <iomanip>
#include <ctime>
#include "afxres.h"
#include "proxy.h"
#include "cache.h"

#pragma comment(lib,"Ws2_32.lib")

#define MAXSIZE 1024 * 2000                         //发送数据报文的最大长度
#define HTTP_PORT 80                                //http 服务器端口
#define TARGET_URL "http://jwts.hit.edu.cn/"        //钓鱼目标地址    
#define FISH_URL "http://jwes.hit.edu.cn/"          //钓鱼转换URL
#define FISH_HOST "jwes.hit.edu.cn"                 //钓鱼转换host

using namespace std;

void ImportBan(vector<string> bans, string banPath);
BOOL CheckBan(string url, string banPath);
void ErrorHandling(string messages, ProxyParam* &lpParameter);
string ParseHttpHead(char *buffer, HttpHeader *httpHeader);
BOOL ConnectToServer(SOCKET *serverSocket,char *host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);
string GetIfModifiedSinceHeader(time_t lastModifiedTime);



//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public 
// Returns: BOOL
// Qualifier: 初始化套接字
//************************************
BOOL Proxy::InitSocket(){
    //加载套接字库（必须）
    WORD wVersionRequested;
    WSADATA wsaData;
    //套接字加载时错误提示
    int err;
    //版本 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //加载 dll 文件 Scoket 库
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        //找不到 winsock.dll
        printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
        return FALSE;
    }
    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) !=2){
        printf("不能找到正确的 winsock 版本\n");
        WSACleanup();
        return FALSE;
    }

    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == ProxyServer){
        printf("创建套接字失败，错误代码为：%d\n",WSAGetLastError());
        return FALSE;
    }
    
    ProxyServerAddr.sin_family = AF_INET;
    ProxyServerAddr.sin_port = htons(ProxyPort);
    ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    if(bind(ProxyServer,(SOCKADDR*)&ProxyServerAddr,sizeof(SOCKADDR)) == SOCKET_ERROR){
        printf("绑定套接字失败\n");
        return FALSE;
    }

    if(listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR){
        printf("监听端口%d 失败",ProxyPort);
        return FALSE;
    }
    
    return TRUE;
}


//************************************
// Method: GetPort
// FullName: GetProxyPort
// Access: public 
// Returns: int
// Qualifier: 获取代理端口号
//************************************
int Proxy::GetPort(){
    return ProxyPort;
}


//************************************
// Method: Start
// FullName: Start
// Access: public 
// Returns: BOOL
// Qualifier: 启动代理服务器
//************************************
BOOL Proxy::Start(vector<string> bansURL, vector<string> bansIP, BOOL fisded){
    printf("The proxy server is running, listening for ports %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET;
    HANDLE hThread;
    DWORD dwThreadID;
    SOCKADDR_IN clientAddr;  // 客户端地址结构体
    int addrLen = sizeof(clientAddr);

    //导入被ban的URL
    ImportBan(bansURL, banUrlPath);

    //导入被ban的IP
    ImportBan(bansIP, banIPPath);

    //代理服务器不断监听
    while(true){
        acceptSocket = accept(ProxyServer, (SOCKADDR*)&clientAddr, &addrLen);
        
        //跳过被ban的IP
        if (CheckBan(inet_ntoa(clientAddr.sin_addr), banIPPath)){
            cout << "This IP : " << inet_ntoa(clientAddr.sin_addr) << " has been banned" << endl;
            continue;
        }

        ProxyParam* lpProxyParam = new ProxyParam;
        if(lpProxyParam == NULL){
            continue;
        }

        lpProxyParam->clientSocket = acceptSocket;
        lpProxyParam->fished = fisded;
        hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);
        CloseHandle(hThread);
        Sleep(200);
    }
    closesocket(ProxyServer);
    WSACleanup();
    return true;
}

//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public 
// Returns: unsigned int __stdcall
// Qualifier: 线程执行函数
// Parameter: LPVOID lpParameter
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter){

    char Buffer[MAXSIZE];
    char *CacheBuffer;
    Cache *cache = new Cache;

    ZeroMemory(Buffer,MAXSIZE);
    SOCKADDR_IN clientAddr;
    ProxyParam *Parameter = (ProxyParam *)lpParameter;
    int length = sizeof(SOCKADDR_IN);
    int recvSize;
    int ret;

    cout << "================" << " Proxy thread start " << "================" << endl;
    
    //拦截浏览器请求
    recvSize = recv(Parameter->clientSocket, Buffer, MAXSIZE, 0);
    //cout << "The request sent by the user is:" << endl << Buffer << endl;
    if(recvSize <= 0){
        ErrorHandling("receive error", Parameter);
    }

    HttpHeader* httpHeader = new HttpHeader();
    CacheBuffer = new char[recvSize + 1];
    ZeroMemory(CacheBuffer,recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);

    //解析 TCP 报文中的 HTTP 头部
    string method = ParseHttpHead(CacheBuffer, httpHeader);

    cout << "Parsed url:        " << httpHeader->url << endl;
    cout << "Parsed host:       " << httpHeader->host << endl;
 
    //跳过connect方法
    if (method == "CONNECT"){ 
        ErrorHandling("", Parameter);
    }
    delete CacheBuffer;

    //跳过被ban的url
    if (CheckBan(httpHeader->url, Parameter->banUrlPath)){
        ErrorHandling("This url has been banned", Parameter);
    }

    //钓鱼操作
    if (Parameter->fished && strstr(httpHeader->url, TARGET_URL)!= NULL){
        //只针对初始请求进行URL替换
        if(strcmp(httpHeader->url, TARGET_URL) == 0){
            strcpy(httpHeader->url, FISH_URL);
            cout << "*******************************************************" << endl;
            cout << TARGET_URL << " has been converted into " << FISH_URL << endl;
            cout << "*******************************************************" << endl;
        }
        strcpy(httpHeader->host, FISH_HOST);
    }

    //cache 命中
    if (cache->Find(httpHeader->url)){
        string message = cache->Read(httpHeader->url);

        //保存的最后修改时间
        time_t lastModifiedTime = cache->GetLastModifiedTime(httpHeader->url);

        if (lastModifiedTime != 0) {
            
            // 将 If-Modified-Since 头部插入到 HTTP 请求中
            string ifModifiedSince = GetIfModifiedSinceHeader(lastModifiedTime);
            string modifiedRequest(Buffer);
            size_t pos = modifiedRequest.find("\r\n\r\n"); // 找到 HTTP 头部结束的地方
            if (pos != string::npos) {
                string newHeader = "If-Modified-Since: " + ifModifiedSince + "\r\n";
                modifiedRequest.insert(pos + 2, newHeader); // 插入新的 If-Modified-Since 头部
            }

            //与目标服务器建立连接
            if(!ConnectToServer(&Parameter->serverSocket, httpHeader->host)) {
                ErrorHandling("Server connection error()", Parameter);
            }
            printf("Connected host:    %s\n",httpHeader->host);


            // 把修改后的请求拷贝到 Buffer 中
            size_t modifiedLength = modifiedRequest.length();
            if (modifiedLength < MAXSIZE) {
                memset(Buffer, 0, MAXSIZE); // 清空 Buffer
                memcpy(Buffer, modifiedRequest.c_str(), modifiedLength); // 将修改后的请求拷贝到 Buffer
                ret = send(Parameter->serverSocket, Buffer, modifiedLength, 0); //将客户端发送的 HTTP 数据报文直接转发给目标服务器
                if (ret <= 0){
                    ErrorHandling("Send to Server error()", Parameter);
                }
            } else {
                ErrorHandling("Modified request too large", Parameter); // 处理请求长度超出缓冲区
            }

            //等待目标服务器返回数据
            recvSize = recv(Parameter->serverSocket, Buffer, MAXSIZE, 0);
            //cout << "The response returned by the target server is" << endl << Buffer << endl;
            if(recvSize > 0){
                // 检查服务器响应是否为 304 Not Modified
                if (strstr(Buffer, "304 Not Modified")) {
                    // 继续使用缓存中的数据
                    cout << endl;
                    cout << "***********************************" << endl;
                    cout << "*          Using Cache!           *" << endl;
                    cout << "***********************************" << endl;
                    cout << endl;
                    memcpy(Buffer, message.data(), message.size());
                } else {
                    // 服务器返回了新的数据，更新缓存
                    cache->Save(httpHeader->url, Buffer, recvSize);
                    cout << endl;
                    cout << "***********************************" << endl;
                    cout << "*          Update DATA!           *" << endl;
                    cout << "***********************************" << endl;
                    cout << endl;
                }
            }
        }
    }

    //cache 未命中
    else{
        //与目标服务器建立连接
        if(!ConnectToServer(&Parameter->serverSocket, httpHeader->host)) {
            ErrorHandling("Server connection error()", Parameter);
        }
        printf("Connected host:    %s\n",httpHeader->host);

        // 将客户端发送的 HTTP 请求转发给目标服务器
        ret = send(Parameter->serverSocket, Buffer, strlen(Buffer), 0);
        if (ret <= 0) {
            ErrorHandling("Send to Server error()", Parameter);
        }

        // 等待目标服务器返回数据
        recvSize = recv(Parameter->serverSocket, Buffer, MAXSIZE, 0);
        //cout << "The response returned by the target server is" << endl << Buffer << endl;
        if (recvSize > 0) {
            // 将服务器返回的数据保存到缓存中
            cache->Save(httpHeader->url, Buffer, recvSize);
            cout << endl;
            cout << "***********************************" << endl;
            cout << "*          SAVE DATA!             *" << endl;
            cout << "***********************************" << endl;
            cout << endl;
        }
    }

    //将收到的数据报转发给客户服务器
    send(Parameter->clientSocket, Buffer, sizeof(Buffer), 0);
    cout << "================" << " Proxy thread end " << "================" << endl << endl;
    _endthreadex(0); //终止线程
    return 0;  
}

//************************************
// Method: ErrorHandling
// FullName: ErrorHandling
// Access: private 
// Returns: void
// Qualifier: 错误处理
// Parameter: messages, lpParameter
//************************************
void ErrorHandling(string messages, ProxyParam* &lpParameter){
    if(messages != ""){
        cout << messages << endl;
    }
    //printf("Close the socket\n");
    Sleep(200);
    closesocket(((ProxyParam*)lpParameter)->clientSocket);
    closesocket(((ProxyParam*)lpParameter)->serverSocket);
    delete lpParameter;
    cout << "================" << " Proxy thread end " << "================" << endl << endl;
    _endthreadex(0);
    exit(0);
}


//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public 
// Returns: void
// Qualifier: 解析 TCP 报文中的 HTTP 头部
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
string ParseHttpHead(char *buffer, HttpHeader *httpHeader){
    char *p;
    char *ptr;
    const char * delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr);//提取第一行
    printf("Request content:   %s\n",p);
    
    if(p[0] == 'G'){//GET 方式
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) -13);
    }else if(p[0] == 'P'){//POST 方式
        memcpy(httpHeader->method,"POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }else if(p[0] == 'C'){ //CONNECT 方法
        return "CONNECT";
    }

    p = strtok_s(NULL, delim, &ptr);
    while(p){
        switch(p[0]){

            case 'H'://Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);
            break;

            case 'C'://Cookie
                if(strlen(p) > 8){
                    char header[8];
                    ZeroMemory(header, sizeof(header));
                    memcpy(header, p, 6);
                    if(!strcmp(header,"Cookie")){
                        memcpy(httpHeader->cookie,&p[8],strlen(p) -8);
                    }
                }
                break;

            default:
                break;
        }
        p = strtok_s(NULL,delim,&ptr);
    }
    return httpHeader->method;
}


//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public 
// Returns: BOOL
// Qualifier: 根据主机创建目标服务器套接字，并连接
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
BOOL ConnectToServer(SOCKET *serverSocket, char *host){
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT *hostent = gethostbyname(host);
    if(!hostent){
        return FALSE;
    }
    in_addr Inaddr=*( (in_addr*) *hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if(*serverSocket == INVALID_SOCKET){
        return FALSE;
    }
    if(connect(*serverSocket,(SOCKADDR *)&serverAddr,sizeof(serverAddr)) == SOCKET_ERROR){
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}

//************************************
// Method: GetIfModifiedSinceHeader
// FullName: GetIfModifiedSinceHeader
// Access: public 
// Returns: string
// Qualifier: 将 time_t 转换为对应If-Modified-Since头部格式
// Parameter: time_t lastModifiedTime
//************************************
string GetIfModifiedSinceHeader(time_t lastModifiedTime) {
    // 将 time_t 转换为 tm 结构
    struct tm* gmtTime = gmtime(&lastModifiedTime);

    // 定义存储日期的缓冲区
    char buffer[100];
    
    // 使用 strftime 格式化时间为 RFC 1123 格式: "Sun, 06 Nov 1994 08:49:37 GMT"
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
    
    // 返回格式化的时间字符串
    return string(buffer);
}

//************************************
// Method: ImportBan
// FullName: ImportBan
// Access: public 
// Returns: void
// Qualifier: 导入被ban的内容
// Parameter: vector<string> bans, string banPath
//************************************
void ImportBan(vector<string> bans, string banPath){
    //将bans写入ban.txt
    ofstream outFile(banPath, ios::binary | ios::out);
    if (!outFile) {
        printf("Error opening file for writing\n");
    }

    // 遍历每个 ban 列表中的字符串，并写入文件
    for(const string& ban : bans) {  
        outFile.write(ban.c_str(), ban.length());
        char newline = '\n';
        outFile.write(&newline, sizeof(newline));
    }
    
    outFile.close();
    if (!outFile) {
        printf("Error writing to file\n");
    }
}

//************************************
// Method: CheckBan
// FullName: CheckBan
// Access: public 
// Returns: BOOL
// Qualifier: 检查该content是否被ban
// Parameter: string content, string banPath
//************************************
BOOL CheckBan(string content, string banPath){
    
    ifstream inFile(banPath, ios::binary | ios::in);
    if (!inFile.is_open()){
        throw string("File read error!");
    }else{
        string line;
        // 逐行读取文件
        while (getline(inFile, line)) {
            // 比较当前行的内容与目标 URL
            if (line == content) {
                return true;  // 找到 URL，返回 true
            }
        }
    }
    return false;
}