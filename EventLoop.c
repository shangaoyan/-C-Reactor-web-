#include "EventLoop.h"
#include <unistd.h>
#include <assert.h>
struct EventLoop *eventLoopInit() // 主线程初始化
{
    return eventLoopInitEx(NULL);
}
// 写数据
void taskWakeup(struct EventLoop *evLoop)
{
    const char *msg = "嘎嘎和哈哈--》尕哈";
    write(evLoop->socketPair[0], msg, strlen(msg));
}
// 读数据
int readLocalMessage(void *arg)
{
    struct EventLoop *evLoop = (struct EventLoop *)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf));
    return 0;
}
struct EventLoop *eventLoopInitEx(const char *threadName) // 子线程初始化
{
    struct EventLoop *evLoop = (struct EventLoop *)malloc(sizeof(struct EventLoop));
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL); // 第二个参数是互斥🔓的属性
    strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);
    //选择使用poll select epoll
    evLoop->dispatcher = &SelectDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();
    // 链表
    evLoop->head = evLoop->tail = NULL;
    evLoop->channelMap = channelMapInit(128); // 初始化channelMap

    // SOCKETPAIR
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    if (ret == -1)
    {
        perror("socketPair error");
        exit(0);
    }
    // 指定规则:evLoop->socketPair[0] 发送数据 ,evLoop->socketPair[1]接收数据
    // 通过在子线程初始化的时候提前往任务队列中放入一个本地任务，这样通过本地发送消息（启动读端）就可以唤醒该子线程
    // 解除子线程的阻塞，去处理任务，通过主线程唤醒子线程
    struct Channel *channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL, NULL, evLoop);
    // channel添加到任务队列
    eventLoopAddTask(evLoop, channel, ADD);

    //
    return evLoop;
}
int eventLoopRun(struct EventLoop *evLoop)
{
    assert(evLoop != NULL);
    // 取出事件分发和检测模型
    struct Dispatcher *dispatcher = evLoop->dispatcher;
    // 比较线程ID是否正常
    if (evLoop->threadID != pthread_self())
    {
        return -1;
    }
    while (!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop, 2); // 开始循环  子线程阻塞在这里!!!
        eventLoopProcessTask(evLoop);    // 当子线程被唤醒后，从阻塞状态解除，执行eventLoopProcessTask,处理任务
    }
}
int eventActivate(struct EventLoop *evLoop, int fd, int event)
{
    if (fd < 0 || evLoop == NULL)
    {
        return -1;
    }
    // 取出Channel
    struct Channel *channel = evLoop->channelMap->list[fd]; // 找到对应的channel
    assert(channel->fd == fd);
    if (event & ReadEvent)
    {
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}
int eventLoopAddTask(struct EventLoop *evLoop, struct Channel *channel, int type)
{
    // 加锁，保护共享资源
    pthread_mutex_lock(&evLoop->mutex);
    // 创建新节点
    struct ChannelElement *node = (struct ChannelElement *)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // 链表为空
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        evLoop->tail->next = node;
        evLoop->tail = node;
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // 处理节点
    // 添加链表节点，可能是当前线程也可能是其他线程（主线程）
    // 修改fd的事件，当前子线程发起，当前子线程处理
    // 添加新的fd，添加任务节点的操作是由主线程发起的
    // 不能让主线程处理任务队列，需要由当前的子线程去处理
    if (evLoop->threadID == pthread_self())
    {
        // 当前子线程
        eventLoopProcessTask(evLoop);
    }
    else
    {
        // 主线程，告诉子线程处理任务队列中的任务
        // 1.子线程在工作，2.子线程被阻塞了:select poll epoll
        taskWakeup(evLoop); // 解除子线程的阻塞-----》唤醒子线程
    }
    return 0;
}
int eventLoopProcessTask(struct EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);
    // 取出头节点
    struct ChannelElement *head = evLoop->head;
    while (head != NULL)
    {
        // 处理任务队列中的每一个任务,这里的任务指的是:
        // 将任务节点对应的文件描述符fd add/delete/modify 到对应子线程的（监听集合）disptch中
        // 从而在该文件描述符触发对应读/写事件后，调用相应的回调函数处理
        struct Channel *channel = head->channel;
        if (head->type == ADD)
        {
            // 添加
            eventLoopAdd(evLoop, channel);
        }
        else if (head->type == DELETE)
        {
            // 删除
            eventLoopRemove(evLoop, channel);
        }
        else if (head->type == MODIFY)
        {
            // 修改
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement *tmp = head;
        head = head->next;
        free(tmp);
    }
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}
int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        // 没有足够的空间存储键值对 fd--channel ==>扩容
        if (!makeMapRoom(channelMap, fd, sizeof(struct Channel *)))
        {
            return -1;
        }
    }
    // 找到fd对应的数组元素位置并存储
    if (channelMap->list[fd] == NULL)
    {
        channelMap->list[fd] = channel;
        evLoop->dispatcher->add(channel, evLoop); // 将该文件描述符fd加入到对于模型select/poll/epoll的监听集合中
    }
    return 0;
}
int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}
int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size || channelMap->list[fd] == NULL)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}
int destorychannel(struct EventLoop *evLoop, struct Channel *channel)
{
    // 删除channel和fd的对应关系
    evLoop->channelMap->list[channel->fd] = NULL;
    // 关闭fd
    close(channel->fd);
    free(channel);
    return 0;
}