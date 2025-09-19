#include "ThreadPool.h"
struct ThreadPool *threadPoolInit(struct EventLoop *mainLoop, int count)
{
    struct ThreadPool *pool = (struct ThreadPool *)malloc(sizeof(struct ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->threadNum = count;
    pool->workerThreads = (struct WorkerThread *)malloc(sizeof(struct WorkerThread ) * count);
    return pool;
}
void threadPoolRun(struct ThreadPool *pool)
{
    assert(pool && !pool->isStart);
    if (pool->mainLoop->threadID != pthread_self())
    {
        perror("ThreadPool error");
        exit(0);
    }
    pool->isStart = true;
    if (pool->threadNum)
    {
        for (int i = 0; i < pool->threadNum; i++)
        {
            workerThreadInit(&pool->workerThreads[i], i); // 初始化子线程
            workerThreadRun(&pool->workerThreads[i]);     // 启动子线程
        }
    }
}
struct EventLoop *takeWorkerEventLoop(struct ThreadPool *pool)
{
    assert(pool->isStart);
    if (pool->mainLoop->threadID != pthread_self()) // 判断是不是主线程
    {
        exit(0);
    }
    // 从线程池中找一个子线程，然后取出里边的反应堆实例
    struct EventLoop *evLoop = pool->mainLoop;
    if (pool->threadNum > 0)
    {
        evLoop = pool->workerThreads[pool->index].evLoop;
        pool->index = (pool->index + 1) % pool->threadNum;
    }
    return evLoop;
}