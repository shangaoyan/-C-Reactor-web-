#include "channel.h"
struct Channel *channelInit(int fd, int events, handlefunc readFunc, handlefunc writeFunc,handlefunc destoryFunc, void *arg)
{
    struct Channel *channel = (struct Channel *)malloc(sizeof(struct Channel));
    channel->arg = arg;
    channel->events = events;
    channel->fd = fd;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->destoryCallback=destoryFunc;
    return channel;
}
void writeEventEnable(struct Channel *channel, bool flag)
{
    if (flag == true)
    {
        channel->events |= WriteEvent;
    }
    else
    {
        channel->events = channel->events & ~WriteEvent; // 给写事件标志位清零
    }
}
bool isWriteEventEnable(struct Channel *channel)
{
    return channel->events & WriteEvent;
}