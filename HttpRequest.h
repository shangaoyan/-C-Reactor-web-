#pragma once
// 请求头键值对
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "Buffer.h"
#include "stdbool.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include "HttpResponse.h"
#include <dirent.h>
#include <strings.h>
#include <assert.h>
#include <string.h>
#include "Log.h"
//#define MSG_SEND_AUTO
struct RequestHeader
{
    char *key;
    char *value;
};
// 当前的解析状态
enum HttpRequestState
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};
// 定义http请求结构体
struct HttpRequest
{
    char *method;
    char *url;
    char *version;
    struct RequestHeader *reqHeaders;
    int regHeadersNum;
    enum HttpRequestState curState;
    int HeaderSize;
};

// 初始化
struct HttpRequest *httpRequestInit();
// 重置
void httpRequestReset(struct HttpRequest *req);
void httpRequestResetEx(struct HttpRequest *req);
void httpRequestDestory(struct HttpRequest *req);
// 获取处理状态
enum HttpRequestState HttpRequestState(struct HttpRequest *request);
// 添加请求头
void httpRequestAddHeader(struct HttpRequest *request, const char *key, const char *value);
// 根据key得到请求头的value
char *httpRequestGetHeader(struct HttpRequest *request, const char *key);
//解析请求行
bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer * readBuf);
//解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request,struct Buffer* readBuf);
//解析http请求协议
bool parseHttpRequest(struct HttpRequest* request,struct Buffer* readBuf,struct HttpResponse* response,struct Buffer* sendBuf,int socket);
//处理http请求协议
bool processHttpRequest(struct HttpRequest* request,struct HttpResponse* response);
//解码函数
void decodeMsg(char* to,char* from);
const char *get_file_type(const char *name);
void sendFile(const char *fileName, struct Buffer* sendBuf,int cfd);
void sendDir(const char *dirName, struct Buffer* sendBuf,int cfd);
int hexToDec(char c);