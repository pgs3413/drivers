#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    
    int fd = open("/home/pan/fs/prfs/root2", O_RDWR);
    if(fd == -1)
    {
        perror("open");
        return 0;
    }

    printf("good!\n");
    return 0;
}