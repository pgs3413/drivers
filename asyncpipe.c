#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>

int fd;
char buf[4096];

void io_handler(int signal)
{
    int ret = read(fd, buf, 4096);
    if(ret >= 0)
    {
        buf[ret] = '\0';
        printf("read async msg: %s\n", buf);
    } else {
        printf("read async msg fail\n");
        perror("err");
    }
}

int main()
{

    fd = open("/dev/scullpipe", O_RDONLY | O_NONBLOCK);

    signal(SIGIO, io_handler);

    fcntl(fd, __F_SETOWN, getpid());
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC);


    printf("do something else ...\n");

    while (1) ;
    
}