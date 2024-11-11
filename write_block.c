#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("wrong argv.\n");
        return 0;
    }

    int fd = open("/dev/sbull", O_WRONLY);
    if(fd < 0)
    {
        perror("open");
        return 0;
    }

    int offset = atoi(argv[1]) * 512;
    lseek(fd, offset, SEEK_SET);

    void *buf = malloc(512);
    int len = strlen(argv[2]);
    *((int *)buf) = len;
    memcpy(buf + sizeof(int), argv[2], len);

    int ret = write(fd, buf, sizeof(int) + len);
    if(ret < 0)
    {
        perror("write");
        return 0;
    }

    printf("write successful.\n");

    int fd2 = open("/dev/sbull", O_RDONLY);
    if(fd2 < 0)
    {
        perror("open");
        return 0;
    }

    lseek(fd2, offset, SEEK_SET);

    char *buf2 = malloc(512);

    ret = read(fd2, buf2, 512);
    if(ret < 0)
    {
        perror("read");
        return 0;
    }

    len = *((int*)buf2);
    buf2[sizeof(int) + len] = '\0';

    printf("msg: %s\n", buf2 + sizeof(int));

    printf("press any key...\n");
    char c;
    scanf("%c", &c);

    return 0;
}