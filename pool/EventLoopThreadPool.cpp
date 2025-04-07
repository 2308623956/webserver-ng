#include <memory>
#include <EventLoopThreadPool.h>
#include <EventLoopThread.h>
#include <Logger.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0), next_(0), hash_(3)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
        // 将线程索引添加到一致哈希中。
        hash_.addNode(std::to_string(i));              
    }

    if (numThreads_ == 0 && cb) // 整个服务端只有一个线程运行baseLoop
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
EventLoop *EventLoopThreadPool::getNextLoop(const std::string &key)
{
    try {
        // 获取节点名称（如 "0"）
        std::string node = hash_.getNode(key);
        // 将节点名称转换为索引
        size_t index = std::stoi(node);
        if (index >= loops_.size()) {
            LOG_ERROR << "Invalid index from consistent hash";
            return baseLoop_;
        }
        LOG_DEBUG << "Get loop " << index << " for key " << key;
        return loops_[index];
    } catch (const std::runtime_error& e) {
        LOG_ERROR << "Consistent hash error: " << e.what();
        return baseLoop_;
    } catch (const std::invalid_argument& e) {
        LOG_ERROR << "Invalid node name format: " << e.what();
        return baseLoop_;
    }
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}