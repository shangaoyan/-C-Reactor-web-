#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"
//#define MSG_SEND_AUTO
struct TcpConnection // TcpConnection是属于evLoop
{
   struct EventLoop *evLoop;
   struct Channel *channel;
   struct Buffer *readBuf;
   struct Buffer *writeBuf;
   char name[32];
   // http协议
   struct HttpRequest *request;
   struct HttpResponse *response;
};
// 初始化
struct TcpConnection *tcpConnectionInit(int fd, struct EventLoop *evLoop);
int processRead(void *arg);
int processWrite(void *arg);
int tcpConnectionDestroy(void *arg);