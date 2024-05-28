### OSH LAB3 实验报告

**选做**

完成了选做部分的**使用线程池机制**和**使用 I/O 复用或异步 I/O（select）**

**编译运行方法**

`make`：生成线程池版本的server和I/O复用版本的server_select
`make server`：只生成线程池版本的server
`make server_select`：只生成I/O复用版本的server_select
`make clean`：清除所有中间文件和可执行文件
`./server`：运行线程池版本的server
`./server_select`：运行I/O复用版本的server_select

**整体设计**

**server.c**中的**handle_clnt**
读取客户端发送来的数据，解析，能够读取相关文件并返回对应内容（200），处理非GET请求/目录文件/非普通文件/路径过长（500），处理找不到文件/打开失败（404）。
通过设置循环和使用缓存变量，能够多次读取或多次写回直到读写完成。

**线程池**

线程池结构体，初始化线程池、创建指定函数的线程、任务入队、释放线程池、销毁线程池。

```c
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
```

**I/O复用（select）**

使用select将检测读缓冲区的任务交给内核。select检测到用作监听的serv_sock的读缓冲区有数据待读取（即有客户端向服务器发起请求）后，accept接收请求，并将返回的文件标识符加入到select检测的集合中，下次检测并处理后移除。select检测直接判断serv_sock的读缓冲区是否有数据，因此确保了accept不会阻塞。

**使用 siege测试的结果和分析**

不使用多线程技术的阻塞版本，测试结果大致在两个相差较大的情况附近，下述前一种情形的效率较高。

```shell
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         1.06,
        "data_transferred":                     0.00,
        "response_time":                        0.05,
        "transaction_rate":                   471.70,
        "throughput":                           0.00,
        "concurrency":                         22.42,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  1.06,
        "shortest_transaction":                 0.00
}
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         3.11,
        "data_transferred":                     0.00,
        "response_time":                        0.08,
        "transaction_rate":                   160.77,
        "throughput":                           0.00,
        "concurrency":                         12.58,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  3.10,
        "shortest_transaction":                 0.00
}
```

**线程池版本**

反复测试发现，测试结果大致在两个相差较大的情况附近，下述第一种情形的效率惊人且多次测试后出现频率仍高于50%，后一种情形相对不使用多线程技术的阻塞版本也有一定性能提升。整体相对阻塞版本有显著的性能提升。

```shell
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         0.03,
        "data_transferred":                     0.00,
        "response_time":                        0.00,
        "transaction_rate":                 16666.67,
        "throughput":                           0.00,
        "concurrency":                         14.00,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  0.02,
        "shortest_transaction":                 0.00
}
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         1.08,
        "data_transferred":                     0.00,
        "response_time":                        0.01,
        "transaction_rate":                   462.96,
        "throughput":                           0.00,
        "concurrency":                          2.74,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  1.05,
        "shortest_transaction":                 0.00
}
```

**I/O复用（select）**

反复测试发现，测试结果大致在两个相差较大的情况附近，多次测试后稳定在后一种性能较差的情况。整体相对阻塞版本有显著的性能提升。

```shell
{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         0.03,
        "data_transferred":                     0.00,
        "response_time":                        0.00,
        "transaction_rate":                 16666.67,
        "throughput":                           0.00,
        "concurrency":                         11.67,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  0.01,
        "shortest_transaction":                 0.00
}
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         1.05,
        "data_transferred":                     0.00,
        "response_time":                        0.02,
        "transaction_rate":                   476.19,
        "throughput":                           0.00,
        "concurrency":                          7.28,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  1.02,
        "shortest_transaction":                 0.00
}
```

