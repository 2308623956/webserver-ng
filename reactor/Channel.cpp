#include <sys/epoll.h>

#include <Channel.h>
#include <EventLoop.h>
#include <Logger.h>

const int Channel::kNoneEvent = 0; //空事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; //读事件
const int Channel::kWriteEvent = EPOLLOUT; //写事件

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{
}

Channel::~Channel()
{
}
// channel的tie方法什么时候调用过?  TcpConnection => channel
/**
 * TcpConnection中注册了Channel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定晚于TcpConnection对象！
 * 此处用tie去解决TcpConnection和Channel的生命周期时长问题，从而保证了Channel对象能够在TcpConnection销毁前销毁。
 **/
//我的理解是 每个chanel的回调函数绑定一个TcpConnection，如果connect结束了，但是chanel还在，而且evenloop还包含并使用chanel，无法找到对应回调函数，问题就很大了
//通过弱引用绑定二者，避免上面问题，然后当loop触发Channel的回调时，先尝试将弱引用提升为强引用，提高对象生命周期
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

//update 和remove => EpollPoller 更新channel在poller中的状态
/**
 * 当改变channel所表示的fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 **/
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中把当前的channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

//利用回调处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    // 检查是否启用了生命周期保护
    if (tied_)
    {
        // 尝试将 weak_ptr 提升为 shared_ptr
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // 打印日志：当前触发的事件类型（用于调试）
    LOG_INFO << "channel handleEvent revents:" << revents_;

    // 处理连接关闭事件（EPOLLHUP 表示挂起，通常对端关闭连接）
    // 注意：排除 EPOLLIN 是为了避免与正常数据读取冲突（如对端关闭写端但仍有数据可读）
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_(); // 触发关闭回调（如清理 TcpConnection 资源）
        }
    }

    // 处理错误事件（EPOLLERR 表示 socket 错误）
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_(); // 触发错误回调（如记录日志、关闭连接）
        }
    }

    // 处理读事件（EPOLLIN 普通数据可读，EPOLLPRI 紧急数据可读）
    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) {
            // 触发读回调，并传递事件发生的时间戳（用于数据接收时间记录）
            readCallback_(receiveTime);
        }
    }

    // 处理写事件（EPOLLOUT 表示 socket 可写）
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_(); // 触发写回调（如发送缓冲区数据）
        }
    }
}
