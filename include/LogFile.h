#pragma once
#include "FileUtil.h"
#include <mutex>
#include <memory>
#include <ctime>

class LogFile
{
public:
    // 构造函数
    LogFile(const std::string &basename, off_t rollsize,
            int flushInterval = 3, int checkEveryN_ = 1024);
    // 析构函数
    ~LogFile();

    /**
     * @brief 追加数据到日志文件
     */
    void append(const char *data, int len);

    // @brief 强制将缓冲区数据刷新到磁盘
    void flush();

    // 滚动日志文件
    bool rollFile();

private:
    // 生成日志文件名
    static std::string getLogFileName(const std::string &basename, time_t *now);

    // 在已加锁的情况下追加数据
    void appendInlock(const char *data, int len);

    const std::string basename_; // 文件名称
    const off_t rollsize_;       // 滚动文件大小
    const int flushInterval_;    // 冲刷时间限值，默认3s
    const int checkEveryN_;      // 写数据次数限制，默认1024
    int count_;                  // 写数据次数计数

    std::mutex mutex_;                                // 互斥锁
    time_t startOfPeriod_;                            // 本次写log周期的起始时间(秒)
    time_t lastRoll_;                                 // 上次roll日志文件时间(秒)
    time_t lastFlush_;                                // 上次flush日志文件时间(秒)
    std::unique_ptr<FileUtil> file_;                  // 文件工具
    const static int kRollPerSeconds_ = 60 * 60 * 24; // 每天滚动一次
};
