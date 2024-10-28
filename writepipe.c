#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("args wrong\n");
        return 0;
    }

    int fd = open("/dev/scullpipe", O_WRONLY | O_NONBLOCK);
    int ret = write(fd, argv[1], strlen(argv[1]));
    close(fd);
    if(ret >= 0)
    {
        printf("write successful\n");
    } else {
        printf("write fail\n");
        perror("err");
    }
    return 0;
}