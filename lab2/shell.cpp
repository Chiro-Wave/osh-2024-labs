// IO
#include <iostream>
// std::string
#include <string>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>
// file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
std::vector<std::string> split(std::string s, const std::string &delimiter);
const char *getHomeDirectory();
int main()
{
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);

  // 用来存储读入的一行命令
  std::string cmd;
  size_t i = 0, j = 0, k = 0;
  // 保存 shell 的标准输入文件描述符
  int shell_fd = dup(STDIN_FILENO);
  while (true)
  {
    // 打印提示符
    std::cout << "$ ";

    // 读入一行。std::getline 结果不包含换行符。
    std::getline(std::cin, cmd);

    // 按空格分割命令为单词
    std::vector<std::string> args = split(cmd, " ");
    // // 输出 args 中的每个单词
    // for (const auto &word : args)
    //   std::cout << word << std::endl;

    // 没有可处理的命令
    if (args.empty())
    {
      continue;
    }

    // 退出
    if (args[0] == "exit")
    {
      if (args.size() <= 1)
      {
        return 0;
      }

      // std::string 转 int
      std::stringstream code_stream(args[1]);
      int code = 0;
      code_stream >> code;

      // 转换失败
      if (!code_stream.eof() || code_stream.fail())
      {
        std::cout << "Invalid exit code\n";
        continue;
      }

      return code;
    }

    if (args[0] == "pwd")
    {
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)) != NULL)
        std::cout << cwd << std::endl;
      else
        perror("getcwd() error");
      continue;
    }

    if (args[0] == "cd")
    {
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)) == NULL) // 获取当前工作目录
        perror("getcwd() error");
      if (args.size() == 1) // cd在没有第二个参数时，默认进入家目录
        chdir(getHomeDirectory());
      else
        chdir(args[1].c_str());
      continue;
    }

    // 处理外部命令
    // std::vector<std::string> 转 char **
    char *arg_ptrs[args.size() + 1][args.size() + 1];
    // 上一个管道的读取端和写入端
    // 关闭：-1；开启：文件描述符
    int prev_pipefd[2] = {-1, -1};
    int pipefd[2];
    int file_r = -1; //< 读
    int file_w = -1; //> 覆写  >> 追加
    for (i = 0, j = 0, k = 0; i < args.size(); i++)
    {
      if (args[i].compare("|") == 0) // 根据'|'来分割命令
      {
        arg_ptrs[j][k] = nullptr; // 使用exec p系列，每条命令的argv都需要以nullptr结尾
        j++, k = 0;
        continue;
      }
      if (args[i].compare(">>") == 0)
      {
        if (i + 1 == args.size())
        {
          perror("Missing target file after \">>\"");
          return 0;
        }
        if (file_w != -1) // 错误检查：有多个>或>>存在，仅最后一个有效，此前打开的文件要关闭
          close(file_w);
        file_w = open(args[++i].c_str(), O_WRONLY | O_APPEND);
        continue;
      }
      if (args[i].compare(">") == 0)
      {
        if (i + 1 == args.size())
        {
          perror("Missing target file after \">\"");
          return 0;
        }
        if (file_w != -1) // 错误检查：有多个>或>>存在，仅最后一个有效，此前打开的文件要关闭
          close(file_w);
        file_w = open(args[++i].c_str(), O_WRONLY | O_TRUNC);
        continue;
      }
      if (args[i].compare("<") == 0)
      {
        if (i + 1 == args.size())
        {
          perror("Missing target file after \"<\"");
          return 0;
        }
        if (file_r != -1) // 错误检查：有多个<存在，仅最后一个有效，此前打开的文件要关闭
          close(file_r);
        file_r = open(args[++i].c_str(), O_RDONLY);
        continue;
      }
      arg_ptrs[j][k] = &args[i][0];
      k++;
    }
    arg_ptrs[j][k] = nullptr; // 最后一条命令也以nullptr结尾
    int command_num = j + 1;  // 分割后的命令数

    for (i = 0; i < command_num; i++)
    {
      // pipefd[0] 存储用于读取的文件描述符，pipefd[1] 存储用于写入的文件描述符
      if (pipe(pipefd) == -1)
        perror("pipe() error");
      // 创建新进程，父进程返回进程标识符process id，子进程此处返回0
      pid_t pid = fork();

      // 仅子进程
      if (pid == 0)
      {
        // 关闭当前管道的读取端
        close(pipefd[0]);
        // 如果是第一条命令，可能需要将标准输入重定向到指定文件
        if (i == 0 && file_r != -1)
          dup2(file_r, STDIN_FILENO);
        // 如果不是第一条命令，则将标准输入重定向到前一个管道的读取端
        if (prev_pipefd[0] != -1)
          dup2(prev_pipefd[0], STDIN_FILENO);
        // 如果不是最后一条命令，则将标准输出重定向到当前管道的写入端
        if (i + 1 != command_num)
          dup2(pipefd[1], STDOUT_FILENO);
        // 如果是最后一条命令，可能需要将标准输出重定向到指定文件
        if (i + 1 == command_num && file_w != -1)
          dup2(file_w, STDOUT_FILENO);
        execvp(arg_ptrs[i][0], arg_ptrs[i]); // 传入
        perror("execvp() error");
        return 0;
        // 子进程结束
      }

      // 仅父进程
      int ret = wait(nullptr);
      if (ret < 0)
        perror("wait failed");
      // 关闭前一个管道的读取端
      if (prev_pipefd[0] != -1)
        close(prev_pipefd[0]);
      // 关闭当前管道的写入端
      close(pipefd[1]);
      // 对于最后一条命令，关闭当前管道的读取端
      if (i + 1 == command_num)
        close(pipefd[0]);
      // 对于最后一条命令，关闭所有已打开的文件
      if (i + 1 == command_num)
      {
        if (file_r != -1)
          close(file_r);
        if (file_w != -1)
          close(file_w);
      }
      // 保存当前管道的读取端和写入端，以便下一次迭代使用
      prev_pipefd[0] = pipefd[0];
      prev_pipefd[1] = pipefd[1];
    }
  }
}
// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter)
{
  std::vector<std::string> res;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    res.push_back(token);
    s = s.substr(pos + delimiter.length());
  }
  res.push_back(s);
  return res;
}
// 获取家目录路径
const char *getHomeDirectory()
{
  // 使用环境变量 HOME 获取用户的家目录
  const char *homeDir = std::getenv("HOME");
  if (homeDir == nullptr)
    perror("getenv(\"HOME\") error");
  return homeDir;
}