#pragma once
#include <string.h>
#include <string>
#include "noncopyable.h"

// 类的前置声明
// //编译器 AsyncLogging 是一个存在的类，避免因互相包含头文件导致的编译错误
// class AsyncLogging;

// 在编译时就能确定其值
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

// 固定的缓冲区类，用于管理日志数据的存储
template <int buffer_size>
class FixedBuffer : noncopyable
{
public:
    // 构造函数，初始化当前指针为缓冲区的起始位置
    FixedBuffer()
        : cur_(data_), size_(0)
    {
    }

    // 将指定长度的数据追加到缓冲区
    // 如果缓冲区有足够的可用空间，则将数据复制到当前指针位置，并更新当前指针
    void append(const char *buf, size_t len)
    {
        if (avail() > len)
        {
            memcpy(cur_, buf, len); // 复制数据到缓冲区
            add(len);
        }
    }

    // 返回缓冲区的起始地址
    const char *data() const
    {
        return data_;
    }

    // 返回缓冲区中当前有效数据的长度
    int length() const
    {
        return size_;
    }

    char *current()
    {
        return cur_;
    }

    size_t avail() const
    {
        return static_cast<size_t>(buffer_size - size_);
    }

    // 更新当前指针，增加指定长度
    void add(size_t len)
    {
        cur_ += len;
        size_ += len;
    }

    // 重置当前指针，回到缓冲区的起始位置
    void reset()
    {
        cur_ = data_;
        size_ = 0;
    }

    // 清空缓冲区的数据
    void bzero()
    {
        ::bzero(data_, sizeof(data_));
    }

    // 将缓冲区中的数据转换为std::string类型并返回
    std::string toString() const
    {
        return std::string(data_, length());
    }

private:
    char data_[buffer_size]; // 定义固定大小的缓冲区
    char *cur_;              // 当前指针，指向缓冲区中下一个可写入的位置
    int size_;               // 缓冲区的大小
};
