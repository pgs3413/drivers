#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    
    int fd = open("/home/pan/fs/prfs/a.txt", O_WRONLY | O_CREAT, 0660);
    if(fd == -1)
    {
        perror("open");
        return 0;
    }

    printf("good!\n");
    return 0;
}