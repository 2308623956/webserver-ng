#pragma once
#include <string>
#include <stdio.h>
#include <sys/types.h>//for off_t 

class FileUtil{
public:
    //构造函数
    FileUtil(std::string& file_name);

    //析构函数
    ~FileUtil();

    //向文件中写入数据
    void append(const char* data, size_t len);

    /**
     * @brief 刷新文件缓冲区
     * 将缓冲区中的数据立即写入文件
     */
    void flush();

    /**
     * @brief 返回已经写入的字节数
     * @return off_t
     */
    off_t writtenBytes() const { return writtenBytes_; }  


private:
    size_t write(const char* data, size_t len);
    //文件指针
    FILE* file_;
    //缓冲区 64kb
    char buffer_[64*1024];
    //已经写入的字节数 off_t是一个长整型
    off_t writtenBytes_;
};