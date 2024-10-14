#ifndef CACHE_H
#define CACHE_H

#include "afxres.h"
#include <string>
#include <vector>
using namespace std;

// Cache类
class Cache{

    private:
        string dirPath = "./temp";              //缓存的默认存储路径
        string UrlToFilename(string url);       //将url转化成合法的文件名

    public:
        //************************************
        // Method: Find
        // FullName: Find
        // Access: public
        // Returns: BOOL
        // Qualifier: 寻找当前存储路径下是否已缓存url内容信息
        // Parameter: string url
        //************************************
        BOOL Find(string url);

        //************************************
        // Method: Save
        // FullName: Save
        // Access: public
        // Returns: void
        // Qualifier: 将信息存储到存储路径下，文件名称为'（转换过的url）.bin',
        //            其中url为传递参数
        // Parameter: string url, const char* message, size_t messageSize
        //************************************
        void Save(string url, const char* message, size_t messageSize);

        //************************************
        // Method: Read
        // FullName: Read
        // Access: public
        // Returns: string
        // Qualifier: 将存储路径下的文件读取出来
        // Parameter: string url
        //************************************
        string Read(string url);
        
        //************************************
        // Method: GetLastModifiedTime
        // FullName: GetLastModifiedTime    
        // Access: public
        // Returns: time_t
        // Qualifier: 跟据url找到对应bin文件的最后修改日期
        // Parameter: string url
        //************************************
        time_t GetLastModifiedTime(string url);

};

#endif