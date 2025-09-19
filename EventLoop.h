#pragma once
#include "Dispatcher.h"
#include "channelMap.h"
#include "channel.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/socket.h>
extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;//使用其他文件的全局变量
//定义任务队列的节点
struct Dispatcher;
struct ChannelElement
{
   int type;//如何处理channel
   struct Channel *channel;
   struct ChannelElement *next;
};
enum ElemType{ADD,DELETE,MODIFY};
struct EventLoop
{
   bool isQuit;
   struct Dispatcher * dispatcher;
   void * dispatcherData;
   //任务队列
   struct ChannelElement *head;
   struct ChannelElement *tail;
   //map
   struct ChannelMap * channelMap;
   //线程ID
   pthread_t threadID;
   char threadName[32];
   pthread_mutex_t mutex;//互斥锁

   int socketPair[2];
};
//初始化
struct EventLoop* eventLoopInit();
struct EventLoop* eventLoopInitEx(const char * threadName);
//启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop);
//处理被激活的文件fd
int eventActivate(struct EventLoop* evLoop,int fd,int event);
//添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel,int type);
//处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop*evLoop);
//任务处理函数
int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop,struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop,struct Channel* channel);
int destorychannel(struct EventLoop*evLoop,struct Channel* channel);
int readLocalMessage(void *arg);