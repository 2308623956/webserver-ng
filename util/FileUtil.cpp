#include <cstring>
#include "FileUtil.h"

// fopen("log.txt", "ae");   以追加模式打开，且防止文件描述符泄漏
FileUtil::FileUtil(std::string &file_name) : file_(::fopen(file_name.c_str(), "ae")), writtenBytes_(0)
{
    // 将file_缓冲区设置为本地缓冲降低io次数。
    ::setbuffer(file_, buffer_, sizeof(buffer_));
}

FileUtil::~FileUtil()
{
    if (file_)
    {
        ::fclose(file_);
    }
}

void FileUtil::append(const char *data, size_t len)
{
    size_t writen = 0; // 记录已写入的字节数

    while (writen != len) // 循环直到所有数据写入完成
    {
        size_t remain = len - writen; // 计算剩余需要写入的字节数

        // 从 data + writen 处开始写入 remain 个字节
        size_t n = write(data + writen, remain);

        // 如果写入字节数不等于 remain，说明发生了部分写入或写入失败
        if (n != remain)
        {
            int err = ferror(file_); // 检查文件流是否发生错误
            if (err)                 // 如果文件流出错
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", strerror(err)); // 输出错误信息
                clearerr(file_);                                                  // 清除文件错误标志，防止影响后续操作
                break;                                                            // 退出循环，避免死循环
            }
        }
        writen += n; // 更新已写入字节数
    }
    writtenBytes_ += writen; // 累计写入的总字节数
}

void FileUtil::flush()
{
    ::fflush(file_);
}

// 真正向文件写入数据
size_t FileUtil::write(const char *data, size_t len)
{
    // 没用选择线程安全的fwrite()为性能考虑。
    return ::fwrite_unlocked(data, 1, len, file_);
}
