#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>


int main() {
    printf("Hello! PB22111623\n"); // Your Student ID
    char buffer1[50]={'\0'};
    char buffer2[50]={'\0'};
    int sys_hel_val=syscall(548,buffer1,50);
    printf("sys_hello returns %d, when buf_len is enough (=50)\n",sys_hel_val);
    printf("buf:%s\n",buffer1);
    sys_hel_val=syscall(548,buffer2,25);
    printf("sys_hello returns %d, when buf_len is not enough (=25)\n",sys_hel_val);
    printf("buf:%s\n",buffer2);
    
    while (1) {}
}
