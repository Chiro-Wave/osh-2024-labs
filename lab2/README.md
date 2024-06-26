**实现的选做功能**

**实现的选做功能**

1. cd 在没有第二个参数时，默认进入家目录。

   > 通过`std::getenv("HOME")`来获取环境变量"HOME"对应的路径，作为参数传入chdir()即可。

2. 支持Ctrl+D(EOF)关闭shell

   >   if (!std::getline(std::cin, cmd)) // 输入命令，若为Ctrl+D(EOF)，则终止终端

3. 支持`histroy n`  ` !n`  `!!`

   > 测试Ubuntu 22.04.3 LTS的bash，连续的相同命令只会在history中记录一次（包括所有的!!和与上条命令内容相同的!n，即使!n执行的命令和上一条命令的序号不同，只要内容相同就不会再记录），报错的命令也会在history中记录，history n会展示包括自身在内的n条历史命令。本实验实现的这三种命令的效果与Ubuntu 22.04.3 LTS的上述特点相同。

4. 支持文本重定向

   > 创建一个管道，文本末补'\n'后写入管道，关闭管道写入端，命令执行前将标准输入重定向到该管道读取端，实现文本重定向。

**`split`**

基于助教给出的示例，稍作修改，使分割字符的连续空格不会影响命令参数的识别

**`cd`**

chdir()接受一个字符型指针作为路径跳转的参数，不能留空。相对路径和绝对路径都被接受。
**选做** `cd` 在没有第二个参数时，默认进入家目录。通过`std::getenv("HOME")`来获取环境变量"HOME"对应的路径，作为参数传入chdir()即可。

**`重定向`**

使用`<`但指定文件或路径不存在时会打印报错信息并跳过此命令，sheel进入下个循环，等待输入。
使用`>`或`>>`但指定文件或路径不存在时会创建对应文件并完成命令。
使用`<`、`>`或`>>`但缺少文件名参数或因其他原因文件打开失败，会打印报错信息并跳过此命令，shell进入下个循环，等待输入

> 考虑重定向和管道的组合使用在Ubuntu中的实际效果
>
> 正确的写法：命令1 < 输入文件 | 命令2 ...
> 命令1的输入重定向到输入文件，输出重定向到管道
>
> 错误的写法：命令1 | 命令2  ... < 输入文件
> 命令2的输入先重定向到管道，再重定向到输入文件，实际上命令1向管道写入的数据没有被使用
>
> 本实验认为在一行命令中至多使用重定向加入一个输入文件，且输入文件输入到第一个命令的进程。

>考虑错误写法在Ubuntu中实际导致的结果
>
>命令1 > 输出文件1 > 输出文件2
>'>' 对应打开文件并清空其原有内容，文件1和2都会被清空。上述错误写法先将命令1的输出重定向>到文件1，再重定向到文件2，最终只有文件2写入了命令的输出，而文件1只被清空。
>
>命令1 >>输出文件1 >>输出文件2
>'>>'对应打开文件并将写入位置移动到文件末尾，文件1和2都不会被清空。上述错误写法先将命令>1的输出重定向到文件1，再重定向到文件2，最终只有文件2写入了命令的输出，而文件1不动。
>
>这种错误写法可能导致文件1被打开但是没有关闭，本实验中考虑了这一点，通过检测是否已有打开的接收输出的文件、如有则关闭来防止文件未关闭的隐患。
>
>```c++
> if (args[i].compare(">>") == 0)
> {
>   if (i + 1 == args.size())
>   {
>     std::cout << ("Missing target file after \">>\"") << std::endl;
>     file_error = true;
>     break;
>   }
>   if (file_w != -1) // 错误检查：有多个>或>>存在，仅最后一个有效，此前打开的文件要关闭
>     close(file_w);
>   file_w = open(args[++i].c_str(), O_WRONLY | O_APPEND | O_CREAT);
>   if (file_w == -1)
>   {
>     perror("file open error: ");
>     file_error = true;
>     break;
>   }
>   continue;
> }
>```

**`信号处理`**
`CTRL+C` 正确终止正在运行的进程
`CTRL+C` 在 shell 嵌套时也能正确终止正在运行的进程
`CTRL+C` 可以丢弃命令

使用signal()函数实现，第一个参数指定信号，第二个参数传入一个函数指针，可选择库自带的处理方式或自定义函数。

```c++
signal(SIGINT, signal_handler);
```

**`前后台进程`**
本实验实现的wait内建命令不支持Ctrl+C终止。

> **疑难杂症**
>
> windows下复制获得的Makefile在移动到linux系统后报错缺少分割符。使用cat -e -t -v Makefile可以查看Makefile，并显示特殊字符的转义形式。windows下使用CRLF换行，linux下使用LF换行，vscode界面右下角可以快速转换。vscode中使用Tab可设置使用制表符/空格，Makefile要求使用制表符，默认的空格缩进会导致报错缺少分割符。

