#include "TcpServer.h"
#include "TcpConnection.h"
#include "Log.h"
struct TcpServer *tcpServerInit(unsigned short port, int threadNum)
{
    struct TcpServer *tcp = (struct TcpServer *)malloc(sizeof(struct TcpServer));
    tcp->listener = listenerInit(port);
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
    return tcp;
}
struct Listener *listenerInit(unsigned short port)
{
    struct Listener *listener = (struct Listener *)malloc(sizeof(struct Listener));
    // 1.创建监听的lfd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        return NULL;
    }
    // 2.设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("setsockopt error");
        return NULL;
    }
    // 3.绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind error");
        return NULL;
    }
    // 4.设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen error");
        return NULL;
    }
    // 5.返回fd
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}
void tcpServerRun(struct TcpServer *server)
{
    // 启动线程池
    threadPoolRun(server->threadPool);
    // 添加检测的任务
    // 添加一个channel实例,主线程（监听者channel实例）
    struct Channel *channel = channelInit(server->listener->lfd, ReadEvent, acceptConnection, NULL, NULL, server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    // 启动反应堆模型
    eventLoopRun(server->mainLoop);
    Debug("服务器程序已经启动了....");
}
int acceptConnection(void *arg)
{
    // 接收客户端连接
    struct TcpServer *server = (struct TcpServer *)arg;
    int cfd = accept(server->listener->lfd, NULL, NULL);
    // 从线程池中取出某个子线程的反应堆实例，去处理这个cfd
    struct EventLoop *evLoop = takeWorkerEventLoop(server->threadPool);
    // 将cfd放到TcpConnection中进行处理
    tcpConnectionInit(cfd, evLoop);
    return 0;
}