#pragma once

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <errno.h>
#include "LogStream.h"
#include <functional>
#include "Timestamp.h"

#define OPEN_LOGGING

class SourceFile
{
public:
    const char *data_;
    int size_;
    explicit SourceFile(const char *filename) : data_(filename)
    {
        /**
         * 找出data中出现/最后一次的位置，从而获取具体的文件名
         * 2022/10/26/test.log
         */
        const char *slash = strrchr(filename, '/');
        if (slash)
        {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }
};

class Logger
{

public:
    // 枚举类型
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT,
    };
    Logger(const char *filename, int line, LogLevel level);
    ~Logger();

    // 流是会改变的
    LogStream &stream() { return impl_.stream_; }

    // 输出函数和刷新缓冲区函数
    using OutputFunc = std::function<void(const char *msg, int len)>;
    using FlushFunc = std::function<void()>;
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

// 内部类
private:
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        // 构造函数
        Impl(LogLevel level, int savedErrno, const char *filename, int line);
        void formatTime();
        void finish(); // 添加一条log消息的后缀

        // 成员变量
        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

private:
    Impl impl_;
};

// 获取errno信息
const char *getErrnoMsg(int savedErrno);
/**
 * 当日志等级小于对应等级才会输出
 * 比如设置等级为FATAL，则logLevel等级大于DEBUG和INFO，DEBUG和INFO等级的日志就不会输出
 */
// ​定义出临时 Logger 对象
#ifdef OPEN_LOGGING
#define LOG_DEBUG Logger(__FILE__, __LINE__, Logger::DEBUG).stream()
#define LOG_INFO Logger(__FILE__, __LINE__, Logger::INFO).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#else
#define LOG(level) LogStream()
#endif


