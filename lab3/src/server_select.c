#include "server.h"

// 读取客户端发送来的数据，并解析
void handle_clnt(int clnt_sock)
{
    // 将 clnt_sock 作为一个文件描述符，读取最多 MAX_RECV_LEN 个字符
    char *req = (char *)malloc(MAX_RECV_LEN * sizeof(char));
    if (req == NULL)
        Error("malloc error of req");
    // 但一次读取并不保证已经将整个请求读取完整，每次读取的内容暂存到req_buf
    char *req_buf = (char *)malloc(MAX_RECV_LEN * sizeof(char));
    if (req_buf == NULL)
        Error("malloc error of req_buf");

    req[0] = '\0';
    ssize_t req_len = 0;
    ssize_t buf_len = 0;
    int status = 200; // HTTP状态码：200/404/500
    int fd;
    struct stat f_type; // 文件类型

    while (1) // 从clnt_sock多次读取请求直至读取完整，存到req
    {
        buf_len = read(clnt_sock, req_buf, MAX_RECV_LEN - req_len - 1); //-1预留strcat拼接后的\0
        if (buf_len < 0)
            Error("read error of clnt_sock");

        req_buf[buf_len] = '\0'; // strcat要求追加的字符串以\0结尾
        strcat(req, req_buf);    // 将新读取内容追加到req末尾
        req_len = strlen(req);   // 更新req_len

        // 当新读取内容以"\r\n\r\n"结尾时停止读取
        if (req_buf[buf_len - 4] == '\r' && req_buf[buf_len - 3] == '\n' &&
            req_buf[buf_len - 2] == '\r' && req_buf[buf_len - 1] == '\n')
            break;
    }

    if (req_len <= 0)
        Error("request read error");
    else if (req_len < 5 || req[0] != 'G' || req[1] != 'E' || req[2] != 'T' ||
             req[3] != ' ' || req[4] != '/') // 不是GET请求
    {
        status = 500;
    }
    else // 是GET请求
    {
        ssize_t begin = 3;
        req[begin] = '.'; // 构造相对路径最开始的'.'
        ssize_t end = 5;
        int floor = 0; // 文件夹层级
        while (end - begin <= MAX_PATH_LEN && req[end] != ' ')
        { // 找到第一个空格停止
            if (req[end] == '/')
            {
                if (req[end - 1] == '.' && req[end - 2] == '.' &&
                    req[end - 3] == '/')
                    floor--; // 说明到了上级目录
                else
                    floor++;
                if (floor < 0) // 已经在当前路径的上级
                {
                    status = 500;
                    break;
                }
            }
            end++;
        }
        if (end - begin > MAX_PATH_LEN)
            status = 500; // 超过最长路径
        if (status == 200)
        {
            req[end] = '\0';                  // 将空格改为'\0'
            fd = open(req + begin, O_RDONLY); // 以只读方式打开
            if (fd < 0)
                status = 404; // 打开文件失败，返回404
            if (status == 200)
            {
                if (stat(req + begin, &f_type) == -1)
                    Error("stat failed");
                if (S_ISDIR(f_type.st_mode))
                    status = 500; // 是目录文件，500 Error
                if (!S_ISREG(f_type.st_mode))
                    status = 500; // 不是普通文件，500 Error
            }
        }
    }

    // 返回内容
    char *response = (char *)malloc(MAX_SEND_LEN * sizeof(char));
    if (response == NULL)
        Error("malloc error of response");
    size_t response_len;

    switch (status)
    {
    case 200:
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
                HTTP_STATUS_200, (size_t)(f_type.st_size));
        response_len = strlen(response);
        // 通过 clnt_sock 向客户端发送信息
        // 将 clnt_sock 作为文件描述符写内容
        if (write(clnt_sock, response, response_len) == -1)
            Error("write response of 200 failed");
        // 循环读取文件内容并返回文件内容
        while ((response_len = read(fd, response, MAX_SEND_LEN)) != 0)
        {
            if (response_len == -1)
                Error("read file failed");
            if (write(clnt_sock, response, response_len) == -1)
                Error("write file failed");
        }
        break;
    case 404:
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
                HTTP_STATUS_404, (size_t)0);
        response_len = strlen(response);
        if (write(clnt_sock, response, response_len) == -1)
            Error("write response of 404 failed");
        break;
    case 500:
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
                HTTP_STATUS_500, (size_t)0);
        response_len = strlen(response);
        if (write(clnt_sock, response, response_len) == -1)
            Error("write response of 500 failed");
        break;
    default:
        break;
    }

    // 关闭客户端套接字
    close(clnt_sock);
    // 关闭文件
    close(fd);
    // 释放内存
    free(req);
    free(req_buf);
    free(response);
}

int main()
{
    // 创建套接字，参数说明：
    //   AF_INET: 使用 IPv4
    //   SOCK_STREAM: 面向连接的数据传输方式
    //   IPPROTO_TCP: 使用 TCP 协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 设置 SO_REUSEADDR 选项，避免server无法重启
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 将套接字和指定的 IP、端口绑定
    //   用 0 填充 serv_addr（它是一个 sockaddr_in 结构体）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    //   设置 IPv4
    //   设置 IP 地址
    //   设置端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    //   绑定
    bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // 使得 serv_sock 套接字进入监听状态，开始等待客户端发起请求
    listen(serv_sock, MAX_CONN);

    int clnt_sock;                // 接收客户端请求，获得一个可以与客户端通信的新的生成的套接字 clnt_sock
    struct sockaddr_in clnt_addr; // 储存accept返回的客户端的IP和接口信息，本实验中未用到
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    fd_set readset;              // 1024位对应1024fd，select检测其中设为1的fd的读缓冲区是否有数据待读取
    fd_set tmp;                  // 由于每次select会变更readset作为返回值，使用tmp替代
    struct timeval tv = {5, 0};  // select单次检测的最大时间，设为5s+0μs=5s
    int maxfd = serv_sock;       // select所检测的所有文件描述符的最大值，+1即为select的第一个参数，用于限制select检测的范围
    FD_ZERO(&readset);           // readset所有位初始置零
    FD_SET(serv_sock, &readset); // 初始将要检测是否有数据待读取的监听器放入readset

    while (1) // 一直循环
    {
        tmp = readset;
        // select检测待读取集合中是否有数据待读取
        int ret = select(maxfd + 1, &tmp, NULL, NULL, &tv);
        if (!ret) // 5s内没有检测到有数据的读缓冲区，将select的最后一个参数改为NULL也能实现不断等待
            continue;
        if (FD_ISSET(serv_sock, &tmp)) // 服务器套接字有数据待读取
        {
            // select已判断serv_sock的读缓冲区有数据待读取，此处一定不阻塞
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
            // accept返回的clnt_sock也是文件描述符，用于通信，在下个周期由select检测
            FD_SET(clnt_sock, &readset);
            // 更新最大文件描述符
            maxfd = clnt_sock > maxfd ? clnt_sock : maxfd;
        }
        for (int i = 0; i <= maxfd; i++)
        {
            if (i != serv_sock && FD_ISSET(i, &tmp)) // 用于通信的文件描述符的读缓冲区有数据
            {
                // 处理客户端的请求
                handle_clnt(i);
                // 移除已完成的通信的检测
                FD_CLR(i, &readset);
            }
        }
    }

    // 实际上这里的代码不可到达，可以在 while 循环中收到 SIGINT 信号时主动 break
    // 关闭套接字
    close(serv_sock);
    return 0;
}