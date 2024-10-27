#include<sys/ioctl.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include "scull/ioc.h"

int main()
{
    int fd = open("/dev/scull0", O_RDWR);

    size_t size = 0;
    int ret = ioctl(fd, SCULL_GETSIZE, &size);
    if(ret)
    {
        printf("ioctl fail\n");
        return 0;
    } else {
        printf("/dev/scull0 size: %ld\n", size);
    }
       
    printf("clear /dev/scull0 ...\n");
    ret = ioctl(fd, SCULL_CLEAR);
    if(ret)
    {
        printf("clear fail\n");
        perror("err:");
        return 0;
    }

    ioctl(fd, SCULL_GETSIZE, &size);
    printf("clear successful! /dev/scull0 new size: %ld\n", size);

    return 0;
}