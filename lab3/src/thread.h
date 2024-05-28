#include <pthread.h>
#define MAX_THREADS 200 // 线程池最大工作线程个数
#define MAX_QUEUE 1024  // 线程池工作队列上限
#define IMMEDIATE_SHUTDOWN 1
#define GRACEFUL_SHUTDOWN 2
/* 任务结构体 */
typedef struct
{
    void (*function)(int);
    int arg;
} threadpool_task_t;

/* 线程池结构体 */
typedef struct
{
    pthread_mutex_t lock;  // 线程池锁，锁整个的线程池
    pthread_cond_t notify; // 条件变量，用于告知线程池中的线程来任务了

    int thread_num;     // 总线程数
    pthread_t *threads; // 线程
    int started;        // 工作线程数

    threadpool_task_t *queue; // 任务队列，循环队列
    int queue_size;           // 任务队列能容纳的最大任务数
    int task_num;             // 任务队列中剩余的任务个数
    int head;                 // 队头 -> 取任务
    int rear;                 // 队尾 -> 放任务

    int shutdown; // 线程池状态：0.线程池可用；1.立即关闭；2.优雅关闭
} threadpool_t;

threadpool_t *threadpool_init(int thread_num, int queue_size);
void *threadpool_threadfunc(void *threadpool);
void threadpool_add(threadpool_t *pool, void (*function)(int), int arg);
void threadpool_free(threadpool_t *pool);
void threadpool_distory(threadpool_t *pool, int shutdown_mode);