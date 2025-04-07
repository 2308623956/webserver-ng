#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include "AsyncLogging.h"
#include <Logger.h>
// 日志文件滚动大小为1MB (1*1024*1024字节)
static const off_t kRollSize = 1 * 1024 * 1024;

//线程函数 每个线程做的事
void* threadFunc(void* arg) {
    int threadId = *(static_cast<int*>(arg));
    for (int i = 0; i < 100; ++i) {
        LOG_INFO << "Thread " << threadId << " - Count: " << i;
        // 模拟业务处理
        usleep(1000); // 1ms延迟
    }
    return nullptr;
}

//所有线程用的是同一个 AsyncLogging
AsyncLogging *g_asyncLog = NULL;
AsyncLogging *getAsyncLog()
{
    return g_asyncLog;
}

void asyncLog(const char *msg, int len)
{
    AsyncLogging *logging = getAsyncLog();
    if (logging)
    {
        logging->append(msg, len);
    }
}

int main(int argc,char *argv[])
{
    // 第一步启动日志，双缓冲异步写入磁盘.
    // 创建一个文件夹
    const std::string LogDir = "logs";
    mkdir(LogDir.c_str(), 0755);
    // 使用std::stringstream 构建日志文件夹
    std::ostringstream LogfilePath;
    LogfilePath << LogDir << "/" << ::basename(argv[0]); // 完整的日志文件路径
    std::cout << LogfilePath.str() << std::endl;
    AsyncLogging log(LogfilePath.str(), kRollSize);
    g_asyncLog = &log;
    //设置输出函数（静态函数）
    Logger::setOutput(asyncLog); // 为Logger设置输出回调, 重新配接输出位置
    log.start();                 // 开启日志后端线程

    // 创建10个线程
    const int NUM_THREADS = 10;
    std::vector<pthread_t> threads(NUM_THREADS);
    int threadIds[NUM_THREADS];

    // 启动线程
    for (int i = 0; i < NUM_THREADS; ++i) {
        threadIds[i] = i;
        if (pthread_create(&threads[i], nullptr, threadFunc, &threadIds[i]) != 0) {
            std::cerr << "Failed to create thread " << i << std::endl;
            return 1;
        }
    }

    // 等待所有线程完成
    for (auto& tid : threads) {
        pthread_join(tid, nullptr);
    }

    // 保证日志线程处理剩余数据
    sleep(2);

    // 停止日志系统
    log.stop();
    return 0;
}
