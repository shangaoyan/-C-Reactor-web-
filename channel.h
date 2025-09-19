#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
//定义函数指针
typedef int(*handlefunc)(void *arg);
//定义文件描述符的读写事件
enum FDEvent
{
    Timeout=0x01,
    ReadEvent=0x02,
    WriteEvent=0x04
};
struct Channel
{
   int fd;
   int events;
   handlefunc readCallback;
   handlefunc writeCallback;
   handlefunc destoryCallback;
   void *arg;
};
//初始化一个channel
struct Channel * channelInit(int fd,int events,handlefunc readFunc ,handlefunc writeFunc,handlefunc destoryFunc,void *arg);
//修改fd的写事件(检测or不检测)
void writeEventEnable(struct Channel *channel,bool flag);
//判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);
