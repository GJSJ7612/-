#include "cache.h"
#include "afxres.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;

//************************************
// Method: Find
// FullName: Find
// Access: public
// Returns: BOOL
// Qualifier: 寻找当前存储路径下是否已缓存url内容信息
// Parameter: string url
//************************************

BOOL Cache::Find(string url)
{
    DWORD fileAttributes = GetFileAttributesA(dirPath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    { // 路径不存在
        CreateDirectoryA(dirPath.c_str(), NULL);
        return false;
    }
    else
    {
        string fileName = UrlToFilename(url) + ".bin";
        string SearchPath = dirPath + "\\*"; // 用于搜索目录下的所有文件
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA(SearchPath.c_str(), &findFileData);
        do
        {
            if (fileName == findFileData.cFileName)
            {
                FindClose(hFind);
                return true; // 文件找到
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);

        FindClose(hFind);
        return false; // 文件未找到
    }
}

//************************************
// Method: Save
// FullName: Save
// Access: public
// Returns: void
// Qualifier: 将信息存储到存储路径下，文件名称为'url.bin',
//            其中url为传递参数
// Parameter: string url, const char* message, size_t messageSize
//************************************

void Cache::Save(string url, const char* message, size_t messageSize)
{
    DWORD fileAttributes = GetFileAttributesA(dirPath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    { // 路径不存在
        CreateDirectoryA(dirPath.c_str(), NULL);
    }else{
        string filePath = dirPath + "\\" + UrlToFilename(url) + ".bin";
        ofstream outFile(filePath, ios::binary | ios::out);
        outFile.write(message, messageSize);
        outFile.close();
    }
}

//************************************
// Method: Read
// FullName: Read
// Access: public
// Returns: string
// Qualifier: 将存储路径下的文件读取出来
// Parameter: string url
//************************************

string Cache::Read(string url)
{
    string FilePath = dirPath + "\\" + UrlToFilename(url) + ".bin";

    ifstream inFile(FilePath, ios::binary | ios::in);
    if (!inFile.is_open()){
        throw string("File read error!");
    }else{
        // 移动文件指针到文件末尾以计算文件大小
        inFile.seekg(0, ios::end);
        size_t fileSize = inFile.tellg();
        inFile.seekg(0, ios::beg);

        // 创建一个buffer来存储文件内容
        string ret(fileSize, '\0');
        inFile.read(&ret[0], fileSize); // 读取文件内容到ret中
        inFile.close();
        return ret;
    }
}

//************************************
// Method: UrlToFilename
// FullName: UrlToFilename
// Access: private
// Returns: string
// Qualifier: 将url转成合适的bin存储名
// Parameter: string url
//************************************
string Cache::UrlToFilename(string url){
    string filename;
    for (char c : url) {
        switch (c) {
            case '/': filename += "_"; break;
            case '?': filename += "_"; break;
            case ':': filename += ""; break;
            case '\\': filename += "_"; break;
            case '|': filename += "_"; break;
            case '*': filename += "_"; break;
            case '<': filename += "_"; break;
            case '>': filename += "_"; break;
            case '"': filename += "_"; break;
            default: filename += c; break;
        }
    }
    // 截断文件名以符合文件系统限制（如果需要）
    if (filename.length() > 255) {
        filename.erase(255);
    }
    return filename;
}

//************************************
// Method: GetLastModifiedTime
// FullName: GetLastModifiedTime
// Access: public
// Returns: time_t
// Qualifier: 跟据url找到对应bin文件的最后修改日期
// Parameter: string url
//************************************
time_t Cache::GetLastModifiedTime(string url){
    // 获取文件路径
    string filePath = dirPath + "\\" + UrlToFilename(url) + ".bin";
    
    // 文件信息结构体
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    
    // 获取文件属性
    if (!GetFileAttributesExA(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        cerr << "Error getting file attributes for: " << filePath << std::endl;
        return 0; // 文件不存在或出错，返回 0 表示没有修改时间
    }
    
    // 转换文件时间为 time_t 类型
    FILETIME ft = fileInfo.ftLastWriteTime;
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // FILETIME 是以 100 纳秒为单位的时间戳，转换成秒
    time_t lastModifiedTime = (ull.QuadPart / 10000000ULL) - 11644473600ULL; // 1601年到1970年的偏移

    return lastModifiedTime;
}
