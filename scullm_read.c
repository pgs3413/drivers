#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("wrong argv!\n");
        return 0;
    }

    int fd = open("/dev/scullm", O_RDWR);
    if(fd < 0)
    {
        perror("open");
        return 0;        
    }

    char *buf = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, atoi(argv[1]) * 4096);
    if(buf == MAP_FAILED)
    {
        perror("mmap");
        return 0;
    }

    int size = *((int *)buf);

    char buf2[4096];

    memcpy(buf2, buf + sizeof(int), size);

    buf2[size] = '\0';

    printf("msg: %s\n", buf2);

    return 0;
}