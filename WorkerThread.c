#include "WorkerThread.h"
int workerThreadInit(struct WorkerThread *thread, int index)
{
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
}
// 子线程的回调函数
void *subThreadRunning(void *arg)
{
    struct WorkerThread *thread = (struct WorkerThread *)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop=eventLoopInitEx(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);//通知主线程，子线程已经初始化完毕，解除阻塞
    eventLoopRun(thread->evLoop);
    return NULL;
}
void workerThreadRun(struct WorkerThread *thread)//这个函数是主线程调用的！！！
{
    // 创建子线程
    pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
    //因为不确定主线程通过这个函数创建子线程一定可以在主线程往子线程里面添加任务时创建完毕
    //所以需要主线程先在这里阻塞一会儿，等待子线程初始化完毕，才可以添加任务
    //阻塞主线程,让当前函数不会直接结束
    pthread_mutex_lock(&thread->mutex);
    while(thread->evLoop==NULL)
    {
        //函数运行到这里时，如果此时主线程被阻塞，那么互斥🔓会被解开
        pthread_cond_wait(&thread->cond,&thread->mutex);//利用条件变量阻塞主线程
        //当主线程解除阻塞状态后，互斥🔓会被再次锁上
    }
    pthread_mutex_unlock(&thread->mutex); 
}