**`cd`**

chdir()接受一个字符型指针作为路径跳转的参数，不能留空。相对路径和绝对路径都被接受。
**选做** `cd` 在没有第二个参数时，默认进入家目录。通过`std::getenv("HOME")`来获取环境变量"HOME"对应的路径，作为参数传入chdir()即可。

> **疑难杂症**
>
> windows下复制获得的Makefile在移动到linux系统后报错缺少分割符。使用cat -e -t -v Makefile可以查看Makefile，并显示特殊字符的转义形式。windows下使用CRLF换行，linux下使用LF换行，vscode界面右下角可以快速转换。vscode中使用Tab可设置使用制表符/空格，Makefile要求使用制表符，默认的空格缩进会导致报错缺少分割符。

