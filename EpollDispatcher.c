#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#define Max 520
struct EpollData
{
    int epfd;
    struct epoll_event *events;
};
static void *epollInit();
static int epollAdd(struct Channel *channel, struct EventLoop *evLoop);
static int epollRemove(struct Channel *channel, struct EventLoop *evLoop);
static int epollModify(struct Channel *channel, struct EventLoop *evLoop);
static int epollDispatch(struct EventLoop *evLoop, int timeout);
static int epollClear(struct EventLoop *evLoop);
static int epollCtl(struct Channel *channel, struct EventLoop *evLoop, int op);
struct Dispatcher EpollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear};
static int epollCtl(struct Channel *channel, struct EventLoop *evLoop, int op)
{
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd;
    int events = 0;
    // 选择事件
    if (channel->events & ReadEvent)
    {
        events |= EPOLLIN;
    }
    if (channel->events & WriteEvent)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret;
}
static void *epollInit()
{
    struct EpollData *data = (struct EpollData *)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(10);
    if (data->epfd == -1)
    {
        perror("epoll_create error");
        exit(1);
    }
    data->events = (struct epoll_event *)calloc(Max, sizeof(struct epoll_event));
    return data;
}
static int epollAdd(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epollAdd error");
        exit(0);
    }
    return ret;
}
static int epollRemove(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epollRemove error");
        exit(0);
    }
    channel->destoryCallback(channel->arg);
    return ret;
}
static int epollModify(struct Channel *channel, struct EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epollModify error");
        exit(0);
    }
    return ret;
}
static int epollDispatch(struct EventLoop *evLoop, int timeout)
{
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData; // 取出evLoop根节点
    int count = epoll_wait(data->epfd, data->events, Max, 1000 * timeout);
    for (int i = 0; i < count; i++)
    {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        // EPOLLERR 对端断开连接   EPOLLHUP对端端开连接之后继续发数据--->删除事件，连接已断开
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // epollRemove(channel,evLoop);
            continue;
        }
        if (events & EPOLLIN)
        {
            eventActivate(evLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }
    return 0;
}
static int epollClear(struct EventLoop *evLoop)
{
    // 释放evLoop
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}