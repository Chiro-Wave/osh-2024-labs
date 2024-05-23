### OSH LAB3 实验报告

**编译运行方法**

```shell
gcc -o ./src/server server ./src/server.c
./src/server
```

**整体设计**

//TODO

**使用 siege测试的结果和分析**

```shell
yangyibo@LAPTOP-15KPKDOV:~$ siege -c 50 -r 10 http://127.0.0.1:8000/index.html

{       "transactions":                          500,
        "availability":                       100.00,
        "elapsed_time":                         2.10,
        "data_transferred":                     0.00,
        "response_time":                        0.06,
        "transaction_rate":                   238.10,
        "throughput":                           0.00,
        "concurrency":                         14.94,
        "successful_transactions":                 0,
        "failed_transactions":                     0,
        "longest_transaction":                  1.26,
        "shortest_transaction":                 0.00
}
```

成功处理了所有请求