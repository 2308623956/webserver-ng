#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;
/**
 * Channel 类：封装文件描述符（fd）及其事件监听与回调处理，是 Reactor 模式的核心组件。
 * 职责：
 * 1. 管理 fd 关注的事件（读、写等）并通过 Poller 注册/更新。
 * 2. 当事件发生时，触发对应的用户回调（如读数据、关闭连接）。
 * 关键关系：
 * - 每个 Channel 属于一个 EventLoop（one loop per thread）。
 * - EventLoop 通过 Poller 监听多个 Channel 的事件。
 ​**/

 /**chanel 主要是封装了fd 和监听事件发生后的回调处理，可以理解为plooer监听到fd对应事件后，
 告诉eventloop，eventloop然后调用chanel去处理事件，只不过chanel使用与他绑定的回调函数处理。
**/
 class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;              // 通用事件回调类型
    using ReadEventCallback = std::function<void(Timestamp)>; // 读事件回调（带时间戳）

    Channel(EventLoop *loop, int fd); // 构造函数，绑定所属 EventLoop 和 fd
    ~Channel();                       // 析构函数（可能需要注销事件监听）

    // fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用
    void handleEvent(Timestamp receiveTime);

    // 设置各类事件回调函数
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 绑定对象的弱引用，防止 Channel 回调时对象已销毁（如 TcpConnection）
    void tie(const std::shared_ptr<void> &);

    // 获取基本信息
    int fd() const { return fd_; }          // 返回封装的 fd
    int events() const { return events_; }  // 返回当前关注的事件
    void set_revents(int revt) { revents_ = revt; }  // Poller 设置实际发生的事件

    // 设置fd相应的事件状态 相当于epoll_ctl add delete
    //设置事件后，会调用loop去更新chanels，调用plooer去监听这些通道
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // 在 Poller 中的状态标识（kNew/kAdded/kDeleted）
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 返回所属的 EventLoop（用于跨线程检查）
    EventLoop *ownerLoop() { return loop_; }
    // 从 EventLoop 中移除自身（注销事件监听）
    void remove();


private:
    // 内部方法
    void update();                                    // 更新事件到 Poller（调用 EventLoop::updateChannel）
    void handleEventWithGuard(Timestamp receiveTime); // 带线程安全保护的事件处理

    // 静态常量：事件类型标识（对应 epoll 的 EPOLLIN/EPOLLOUT 等）
    static const int kNoneEvent;  // 0x0，无事件
    static const int kReadEvent;  // 0x1，读事件
    static const int kWriteEvent; // 0x4，写事件

    // 成员变量
    EventLoop *loop_; // 所属事件循环（不持有所有权）
    const int fd_;    // 管理的文件描述符（如 socket fd）
    int events_;      // 注册的事件（用户关注的事件） 根据状态 就可以调用对应的函数
    int revents_;     // Poller 返回的就绪事件（实际发生的事件）
    int index_;       // 在 Poller 中的状态标识（kNew/kAdded/kDeleted）

    std::weak_ptr<void> tie_; // 弱引用，绑定到拥有 Channel 的对象（如 TcpConnection）
    bool tied_;               // 是否已绑定 tie_

    // 事件回调函数（由用户设置）
    ReadEventCallback readCallback_; // 读事件回调（需处理时间戳）
    EventCallback writeCallback_;    // 写事件回调
    EventCallback closeCallback_;    // 关闭事件回调（如对端断开）
    EventCallback errorCallback_;    // 错误事件回调（如 socket 错误）
};