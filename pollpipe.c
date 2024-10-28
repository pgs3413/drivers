#include<stdio.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>

int main(int argc, char *argv[])
{

    if(argc != 2)
    {
        printf("args wrong\n");
        return 0;
    }

    int num = atoi(argv[1]);

    int efd = epoll_create1(0);

    for(int i = 0; i < num; i++)
    {
        int fd = open("/dev/scullpipe", O_RDONLY | O_NONBLOCK);
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
        printf("file %d is ready to epoll.\n", fd);
    }

    struct epoll_event evlist[10];
    char buf[4096];

    while (num--)
    {
        printf("epoll wait...\n");
        int cnt = epoll_wait(efd, evlist, 10, -1);

        printf("\n");

        for(int i = 0; i < cnt; i++)
        {
            int fd = evlist[i].data.fd;
            if(!(evlist[i].events & EPOLLIN))
            {
                printf("file %d is not EPOLLIN.\n", fd);
                continue;
            }
            int ret = read(fd, buf, 4096);
            if(ret >= 0)
            {
                buf[ret] = '\0';
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                printf("file %d read msg: %s and then exit.\n", fd, buf);                
            } else {
                printf("file %d read fail. err: %s re epoll.\n", fd, strerror(errno));
            }
        }

        printf("\n");
    
    }
    
    return 0;
}