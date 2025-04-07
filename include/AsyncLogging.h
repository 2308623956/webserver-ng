#pragma once
#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
// #include "LogStream.h"
#include "LogFile.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

class AsyncLogging
{
public:
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }
    // 前端调用append写入日志
    void append(const char *logline, int len);
    void start()
    {
        running_ = true;
        thread_.start();
    }
    void stop()
    {
        running_ = false;
        cond_.notify_one();
    }

private:
    // 私有成员函数
    void threadFunc(); // 后端线程函数

    // 成员变量
    // 固定大小的缓冲区
    using LargeBuffer = FixedBuffer<kLargeBufferSize>;
    using BufferVector = std::vector<std::unique_ptr<LargeBuffer>>;
    // BufferVector::value_type 是 std::vector<std::unique_ptr<Buffer>> 的元素类型，也就是 std::unique_ptr<Buffer>。
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_; // 日志刷新时间
    const std::string basename_;//日志文件的基础名称，用于生成滚动后的日志文件名
    const off_t rollSize_;//日志文件滚动的阈值大小，超过该大小则创建新文件。

    std::mutex mutex_;             // 保护缓冲区的互斥锁
    std::condition_variable cond_; // 通知后端线程的条件变量
    Thread thread_;                // 后端线程对象
    std::atomic<bool> running_;    // 线程运行状态

    BufferPtr currentBuffer_; // 当前端正在写入的缓冲区
    BufferPtr nextBuffer_;    // 备用缓冲区（减少内存分配）
    BufferVector buffers_;    // 待写入文件的缓冲区队列
};