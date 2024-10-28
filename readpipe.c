#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>

int main()
{

    int fd = open("/dev/scullpipe", O_RDONLY);
    char buf[4096];
    int cnt = read(fd, buf, 4096);
    buf[cnt] = '\0';
    printf("read from scullpipe: %s\n", buf);
    close(fd);
    return 0;

}