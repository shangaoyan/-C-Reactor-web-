#define _GNU_SOURCE
#include "HttpRequest.h"
#include <string.h>
#include <strings.h>

//#define HeaderSize 12
struct HttpRequest *httpRequestInit()
{

    struct HttpRequest *request = (struct HttpRequest *)malloc(sizeof(struct HttpRequest));
    httpRequestReset(request);
    request->HeaderSize=12;
    request->reqHeaders = (struct RequestHeader *)malloc(sizeof(struct RequestHeader) * request->HeaderSize);
    return request;
}
void httpRequestReset(struct HttpRequest *req)
{
    req->curState = ParseReqLine;
    req->method = NULL;
    req->url = NULL;
    req->version = NULL;
    req->regHeadersNum = 0;
}
void httpRequestResetEx(struct HttpRequest *req)
{
    if (req->method) free(req->method);
    if (req->url) free(req->url);
    if (req->version) free(req->version);
    if (req->reqHeaders != NULL)
    {
        for (int i = 0; i < req->regHeadersNum; i++)
        {
            free(req->reqHeaders[i].key);
            free(req->reqHeaders[i].value);
        }
        free(req->reqHeaders);
        req->reqHeaders = NULL;
    }
    httpRequestReset(req);
}
void httpRequestDestory(struct HttpRequest *req)
{
    if (req != NULL)
    {
        httpRequestResetEx(req);
    }
    free(req);
}
enum HttpRequestState HttpRequestState(struct HttpRequest *request)
{
    return request->curState;
}
void httpRequestAddHeader(struct HttpRequest *request, const char *key, const char *value)
{
    if (request->regHeadersNum >= request->HeaderSize) {
        // 动态扩容
        int newSize = request->HeaderSize * 2;
        struct RequestHeader *newHeaders = realloc(request->reqHeaders, 
                                                  sizeof(struct RequestHeader) * newSize);
        if (newHeaders == NULL) {
            Debug("错误：无法扩容请求头数组");
            return;
        }
        request->reqHeaders = newHeaders;
        request->HeaderSize=newSize;
        // 更新HeaderSize的值（需要修改结构体设计）
    }
    request->reqHeaders[request->regHeadersNum].key = key;
    request->reqHeaders[request->regHeadersNum].value = value;
    request->regHeadersNum += 1;
}
char *httpRequestGetHeader(struct HttpRequest *request, const char *key)
{
    if (request != NULL)
    {
        for (int i = 0; i < request->regHeadersNum; i++)
        {
            if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0)
            {
                return request->reqHeaders[i].value;
            }
        }
    }
    return NULL;
}
char *splitRequestLine(const char *start, const char *end, const char *sub, char **ptr)
{
    char *space = end;
    if (sub != NULL)
    {
        space = memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    char *tmp = (char *)malloc(length + 1);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}
bool parseHttpRequestLine(struct HttpRequest *request, struct Buffer *readBuf)
{
    Debug("开始解析请求行...");
    // 读出请求行
    char *end = bufferFindCRLF(readBuf);
    // 保存字符串起始地址
    char *start = readBuf->data + readBuf->readPos;
    // 请求行总长度
    int lineSize = end - start;
    if (lineSize)
    {
        // get /xxx/xx.txt http/1.1
        // 请求方式
        start = splitRequestLine(start, end, " ", &request->method);
        Debug("请求方法:%s",request->method);
        // 请求的静态资源
        start = splitRequestLine(start, end, " ", &request->url);
        Debug("请求资源:%s",request->url);
        // http版本
        splitRequestLine(start, end, NULL, &request->version);
        Debug("请求版本:%s",request->version);
        // 为解析请求头作准备
        readBuf->readPos += lineSize;
        readBuf->readPos += 2;
        // 修改状态
        request->curState = ParseReqHeaders;
        return true;
    }
    // 保存字符串结束地址

    return false;
}
bool parseHttpRequestHeader(struct HttpRequest *request, struct Buffer *readBuf)
{
    Debug("开始解析请求头...");
    char *end = bufferFindCRLF(readBuf); // 找到下一行的末尾，为解析请求头作准备
    if (end != NULL)
    {
        char *start = readBuf->data + readBuf->readPos;
        int lineSize = end - start;
        if (lineSize == 0)
        {
            readBuf->readPos += 2;
            request->curState = ParseReqDone;
            return true;
        }
        // 基于:搜索字符串
        char *middle = memmem(start, lineSize, ": ", 2);
        if (middle != NULL)
        {
            char *key = malloc(middle - start + 1);
            strncpy(key, start, middle - start);
            key[middle - start] = '\0';

            char *value = malloc(end - middle - 2 + 1);
            strncpy(value, middle + 2, end - middle - 2);
            value[end - middle - 2] = '\0';

            httpRequestAddHeader(request, key, value);
            // 移动读数据的位置
            readBuf->readPos += lineSize;
            readBuf->readPos += 2;
        }
        else
        {
            // 请求头解析完毕，跳过空行
            readBuf->readPos += 2;
            // 修改解析状态
            // 忽略Post请求，按照get请求处理
            request->curState = ParseReqDone;
        }
        return true;
    }
    return false;
}

bool parseHttpRequest(struct HttpRequest *request, struct Buffer *readBuf, struct HttpResponse *response, struct Buffer *sendBuf, int socket)
{
    bool flag = true;
    Debug("开始解析http请求...");
    while (request->curState != ParseReqDone)
    {
        switch (request->curState)
        {
        case ParseReqLine:
        {
            flag = parseHttpRequestLine(request, readBuf);
            break;
        }
        case ParseReqHeaders:
        {
            flag = parseHttpRequestHeader(request, readBuf);
            break;
        }
        case ParseReqBody:
        {
            break;
        }
        default:
            break;
        }
        if (!flag)
        {
            return flag;
        }
        // 判断是否解析完毕，如果完毕了，需要准备回复的数据
        if (request->curState == ParseReqDone)
        {
            Debug("解析完毕，准备发送数据...");
            // 1.根据解析出的原始数据，对客户端的请求作出处理
            processHttpRequest(request, response);
            // 2.组织响应数据并发送给客户端
            httpResponsePrepareMsg(response, sendBuf, socket);
        }
    }
    request->curState = ParseReqLine;
    return flag;
}
bool processHttpRequest(struct HttpRequest *request, struct HttpResponse *response)
{
    // 解析请求行
    // strcasecmp不区分大小写的比较
    Debug("开始识别...%s",request->method);
    if (strcasecmp(request->method, "GET") != 0)
    {
        return false;
    }
    Debug("识别到为get请求");
    decodeMsg(request->url, request->url);
    // 处理客户端请求的静态文件路径
    char *file = NULL;
    if (strcmp(request->url, "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = request->url + 1;
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    Debug("成功获取文件路径（URL）:%s，本地路径:%s", request->url, file);
    if (ret == -1)
    {
        // 文件不存在 - 404页面
        // sendHeadMsg(cfd, 404, "Not Found", get_file_type(".html"), -1);
        // sendFile("404.html", cfd);
        strcpy(response->filename, "404.html");
        response->statusCode = NotFound;
        strcpy(response->statusMsg, "Not Found");
        // 响应头
        httpResponseAddHeader(response, "Content-type", get_file_type(".html"));
        response->sendDataFunc = sendFile;
        return 0;
    }
    strcpy(response->filename, file);
    response->statusCode = OK;
    strcpy(response->statusMsg, "OK");
    // 判断文件类型
    if (S_ISDIR(st.st_mode))
    {
        // 发送目录内容
        // sendHeadMsg(cfd, 200, "OK", get_file_type(".html"), -1);
        // sendDir(file, cfd);
        // 响应头
        httpResponseAddHeader(response, "Content-type", get_file_type(".html"));
        Debug("即将发送目录数据...");
        response->sendDataFunc = sendDir;
    }
    else
    {
        // 发送文件内容
        // sendHeadMsg(cfd, 200, "OK", get_file_type(file), st.st_size);
        // sendFile(file, cfd);
        // 响应头
        char tmp[12] = {0};
        sprintf(tmp,"%ld", st.st_size);
        httpResponseAddHeader(response, "Content-type", get_file_type(file));
        httpResponseAddHeader(response, "content-length", tmp);
        Debug("即将发送文件数据...");
        response->sendDataFunc = sendFile;
    }
    Debug("分析完毕请求数据，准备发送...");
    return false;
}
void decodeMsg(char *to, char *from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit ->判断字符是不是16进制格式，取值在0-f
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 ->十进制 将这个数值复制给了字符 int -> char
            //  B2=178
            // 将三个字符，变成一个字符，这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
            // 跳过from[1]和from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝，赋值
            *to = *from;
        }
    }
    *to = '\0';
}
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c <= 'z' && c >= 'a')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    return 0;
}
// 通过文件名获取文件的类型
const char *get_file_type(const char *name)
{
    char *dot;
    // 自右向左查找 '.' 字符，如不存在返回 NULL
    dot = strrchr((char *)name, '.');
    if (dot == NULL)
    {
        return "text/plain; charset=utf-8";
    }
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    {
        return "text/html; charset=utf-8";
    }
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    if (strcmp(dot, ".gif") == 0)
    {
        return "image/gif";
    }
    if (strcmp(dot, ".png") == 0)
    {
        return "image/png";
    }
    if (strcmp(dot, ".css") == 0)
    {
        return "text/css";
    }
    if (strcmp(dot, ".au") == 0)
    {
        return "audio/basic";
    }
    if (strcmp(dot, ".wav") == 0)
    {
        return "audio/wav";
    }
    if (strcmp(dot, ".avi") == 0)
    {
        return "video/x-msvideo";
    }
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    {
        return "video/quicktime";
    }
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
    {
        return "video/mpeg";
    }
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
    {
        return "model/vrml";
    }
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
    {
        return "audio/midi";
    }
    if (strcmp(dot, ".mp3") == 0)
    {
        return "audio/mpeg";
    }
    if (strcmp(dot, ".ogg") == 0)
    {
        return "application/ogg";
    }
    if (strcmp(dot, ".pac") == 0)
    {
        return "application/x-ns-proxy-autoconfig";
    }

    // 未匹配到则返回默认文本类型
    return "text/plain; charset=utf-8";
}
void sendFile(const char *fileName, struct Buffer *sendBuf, int cfd)
{
    // 1.打开文件
    int fd = open(fileName, O_RDONLY);
    //assert(fd > 0);
    if (fd == -1)
    { // 替换 assert，处理文件打开失败的情况
        perror("open file error");
        // 发送简单的404响应（避免递归调用sendFile）
        char buf[1024] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>File Not Found</h1>";
        send(cfd, buf, strlen(buf), 0);
        return -1;
    }
#if 1
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof(buf));
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
            bufferSendData(sendBuf, cfd);
#endif
             // 休眠10ms再发，防止浏览器处理不过来发生问题
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read error");
        }
    }
#else
    // lseek函数，将函数指针从0移动到末位，返回字节数，获取文件大小
    off_t offset = 0;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size)
    {
        int ret = sendfile(cfd, fd, &offset, size - offset);
        printf("ret value:%d\n", ret);
        if (ret == -1 && errno == EAGAIN)
        {
            printf("没数据...");
        }
    }
#endif
    close(fd);
    return ;
}
void sendDir(const char *dirName, struct Buffer *sendBuf, int cfd)
{
    char buf[4096] = {0};
    // 拼接html
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
    struct dirent **namelist;
    // scanfdir得到目录数量
    int num = scandir(dirName, &namelist, NULL, alphasort);
    for (int i = 0; i < num; i++) // 遍历所有目录
    {
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName, name);
        Debug("路径:%s",subPath);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href=" ">name</a>    \"这里的\是转义字符的用法，表示"
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        else
        {
            sprintf(buf, "<tr><td><a href=\"%s\">%s</a></td><td>%lld Bytes</td></tr>", name, name, st.st_size);
        }
        // send(cfd, buf, sizeof(buf), 0); // 发送数据到客户端
        bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
        bufferSendData(sendBuf, cfd);
#endif
        bzero(buf, sizeof(buf)); // 清空buf
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
    free(namelist);
    return ;
}