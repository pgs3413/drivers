#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("wrong argv.\n");
        return 0;
    }

    int fd = open("/dev/sbull", O_RDONLY);
    if(fd < 0)
    {
        perror("open");
        return 0;
    }

    int offset = atoi(argv[1]) * 512;
    lseek(fd, offset, SEEK_SET);

    char *buf = malloc(512);

    int ret = read(fd, buf, 512);
    if(ret < 0)
    {
        perror("read");
        return 0;
    }

    int len = *((int*)buf);
    buf[sizeof(int) + len] = '\0';

    printf("read successful. msg: %s\n", buf + sizeof(int));
    return 0;
}