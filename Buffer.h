#pragma once
#define _GUN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <malloc.h>
#include <strings.h>
struct Buffer
{
    //指向内存的指针
    char *data;
    int capacity;
    int readPos;
    int writePos;
};
//初始化
struct Buffer*bufferInit(int size);
//销毁内存
void bufferDestory(struct Buffer*buf);
//扩容
void bufferExtendRoom(struct Buffer*buffer,int size);
//得到剩余可写的内存容量
int bufferReadableSize(struct Buffer*buffer);
//得到剩余可读的内存容量
int bufferWriteableSize(struct Buffer*buffer);
//写内存 1.直接写 2.接收套接字数据
int bufferAppendData(struct Buffer*buffer,const char *data,int size);
int bufferAppendString(struct Buffer* buffer,const char *data);
int bufferSocketRead(struct Buffer *buffer,int fd);

//根据\r\n取出一行，找到其在数据块中的位置，返回该位置
char *bufferFindCRLF(struct Buffer* buffer);
int bufferSendData(struct Buffer* buffer,int socket);