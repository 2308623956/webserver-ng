#include "AsyncLogging.h"
#include <stdio.h>
AsyncLogging::AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval)
    :
      flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(),
      currentBuffer_(new LargeBuffer),
      nextBuffer_(new LargeBuffer),
      buffers_()
{
    currentBuffer_->bzero();  // 清零当前缓冲区
    nextBuffer_->bzero();     // 清零备用缓冲区
    buffers_.reserve(16);     // 预分配队列空间（避免动态扩容）
}

// 调用此函数解决前端把LOG_XXX<<"..."传递给后端，后端再将日志消息写入日志文件
//将前端的buffer数据 存放到buffers_ 缓冲队列
void AsyncLogging::append(const char *logline, int len)
{
    //加锁保证线程安全
    //std::lock_guard<std::mutex> 自动管理互斥锁（mutex）的加锁和解锁
    std::lock_guard<std::mutex> lg(mutex_);
    // 缓冲区剩余的空间足够写入
    if (currentBuffer_->avail() > static_cast<size_t>(len))
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // 将当前缓冲区移动到待处理队列
        buffers_.push_back(std::move(currentBuffer_));
        // 尝试使用备用缓冲区 (nextBuffer_)
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            // 没有备用缓冲区时，新建一个
            currentBuffer_.reset(new LargeBuffer);
        }
        // 将新数据写入新的当前缓冲区
        currentBuffer_->append(logline, len);
        // 唤醒后端线程写入磁盘 有一个缓冲区满了
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    // output写入磁盘接口
    LogFile output(basename_, rollSize_);
    BufferPtr newbuffer1(new LargeBuffer); // 生成新buffer替换currentbuffer_
    BufferPtr newbuffer2(new LargeBuffer); // 生成新buffer2替换newBuffer_，其目的是为了防止后端缓冲区全满前端无法写入
    // 初始化缓冲区
    newbuffer1->bzero();
    newbuffer2->bzero();
    // 缓冲区数组置为16个，用于和前端缓冲区数组进行交换
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        // 步骤1：锁定互斥锁，交换缓冲区
        {
            // 互斥锁保护这样就保证了其他前端线程无法向前端buffer写入数据
            std::unique_lock<std::mutex> lg(mutex_);
            if (buffers_.empty())
            {
                cond_.wait_for(lg, std::chrono::seconds(3));
            }
            buffers_.push_back(std::move(currentBuffer_));// 移动当前缓冲区到队列
            currentBuffer_ = std::move(newbuffer1);       // 移动新缓冲区到当前缓冲区
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newbuffer2);    // 补充前端的备用缓冲区2
            }
            buffersToWrite.swap(buffers_);
        }
        // 步骤2：将缓冲区数据写入磁盘
        // 从待写缓冲区取出数据通过LogFile提供的接口写入到磁盘中
        for (auto &buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }
        // 步骤3：回收并重置缓冲区 注：buffersToWrite里面有数据的
        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }
        // 回收缓冲区1
        if (!newbuffer1)
        {
            // 移动所有权
            newbuffer1 = std::move(buffersToWrite.back());
            // 从队列移除
            buffersToWrite.pop_back();
            //清空buffersToWrite里面的数据
            newbuffer1->reset();
        }
        // 回收缓冲区2
        if (!newbuffer2)
        {
            newbuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newbuffer2->reset();
        }
        buffersToWrite.clear(); // 清空后端缓冲队列
        output.flush();         // 清空文件夹缓冲区
    }
    output.flush(); // 确保一定清空。
}