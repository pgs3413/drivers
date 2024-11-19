#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

int main(int argc, char *argv[])
{

    int fd = open("/home/pan/fs/prfs/root.txt", O_RDONLY);
    if(fd == -1)
    {
        perror("open");
        return 1;
    }

    printf("good!\n");
    return 0;
}