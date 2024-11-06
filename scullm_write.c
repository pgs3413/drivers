#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if(argc < 3)
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

    char *buf = mmap(NULL, 4096, PROT_WRITE, MAP_SHARED, fd, atoi(argv[1]) * 4096);
    if(buf == MAP_FAILED)
    {
        perror("mmap");
        return 0;
    }

    int size = strlen(argv[2]);

    *((int *)buf) = size;

    sprintf(buf + sizeof(int), "%s", argv[2]);

    printf("write to scullm successful\n");

    return 0;
}