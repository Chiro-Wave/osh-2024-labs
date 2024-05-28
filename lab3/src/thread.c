#include "server.h"
#include "thread.h"

// 创建并初始化线程池
threadpool_t *threadpool_init(int thread_num, int queue_size)
{
    threadpool_t *pool = NULL;

    if (thread_num <= 0 || thread_num > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
        Error("threadpool_init parameter error");

    pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (pool == NULL)
        Error("pool malloc error");

    do // 初始化
    {
        pool->thread_num = thread_num;
        pool->started = 0;
        pool->queue_size = queue_size;
        pool->head = pool->rear = pool->task_num = 0;

        pool->shutdown = 0;

        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
        if (pool->threads == NULL)
            Error_Break("pool->threads malloc error");
        memset(pool->threads, 0, sizeof(pthread_t) * thread_num);

        pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);
        if (pool->queue == NULL)
            Error_Break("pool->queue malloc error");

        if (pthread_mutex_init(&pool->lock, NULL) != 0)
            Error_Break("pool->lock init fail");
        if (pthread_cond_init(&pool->notify, NULL) != 0)
            Error_Break("pool->notify init fail");

        int i;
        for (i = 0; i < thread_num; i++)
        {
            if (pthread_create(&pool->threads[i], NULL, threadpool_threadfunc, (void *)pool) != 0)
                Error_Break("pthread_create fail");
            pool->started++; // 开始工作的工作线程个数++
        }
        if (i != thread_num)
            Error_Break("");

        return pool; // 创建和初始化全部成功时，执行此处，返回内存池指针
    } while (0);

    threadpool_free(pool); // 初始化异常，释放内存
    return NULL;
}

/* 线程池中的线程任务 */
void *threadpool_threadfunc(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    for (;;)
    {
        pthread_mutex_lock(&pool->lock);
        while (pool->task_num == 0 && pool->shutdown == 0) // 线程池可用且任务队列为空
        {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        // 若线程池关闭，结束当前线程
        if (pool->shutdown == IMMEDIATE_SHUTDOWN ||
            (pool->shutdown == GRACEFUL_SHUTDOWN && pool->task_num == 0))
            break;

        // 从任务队列中取出任务
        if (pool->task_num == 0)
            pool->head = pool->rear = 0;
        task.function = pool->queue[pool->head].function;
        task.arg = pool->queue[pool->head].arg;

        pool->head = (pool->head + 1) % pool->queue_size;
        pool->task_num--;

        pthread_mutex_unlock(&pool->lock);

        /* 执行任务 */
        (*(task.function))(task.arg);
    }

    pool->started--; // 当前线程退出，正在工作线程个数--
    printf("线程退出\n");

    pthread_mutex_unlock(&pool->lock);
    pthread_exit(NULL);
}

// 向线程池中新增任务
void threadpool_add(threadpool_t *pool, void (*function)(int), int arg)
{
    pthread_mutex_lock(&pool->lock); // 互斥锁加锁防止其他线程同时修改队列状态
    // 若线程池任务队列已满，则等待
    while (pool->task_num == pool->queue_size &&
           pool->shutdown == 0)
        pthread_cond_wait(&pool->notify, &pool->lock);

    // 若线程池不可用，解锁并放弃任务添加
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    // 添加任务到线程池任务队列
    pool->queue[pool->rear].function = function;
    pool->queue[pool->rear].arg = arg;
    pool->rear = (pool->rear + 1) % pool->queue_size;
    pool->task_num++;

    // 通知线程执行任务
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
}

// 释放线程池
void threadpool_free(threadpool_t *pool)
{
    if (pool == NULL)
        return;
    if (pool->threads)
        free(pool->threads);
    if (pool->queue)
        free(pool->queue);
    // 因为我们是在初始化互斥锁和条件变量之后分配pool->threads的，
    // 所以我们可以确信它们已经被正确初始化了。
    // 尽管如此，我们还是会出于保险起见锁定互斥锁
    pthread_mutex_lock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool);

    return;
}

// 销毁线程池
void threadpool_distory(threadpool_t *pool, int shutdown_mode)
{
    if (pool == NULL || pool->shutdown)
        return;

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = shutdown_mode;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->thread_num; i++)
        pthread_join(pool->threads[i], NULL);

    threadpool_free(pool);
}