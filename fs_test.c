#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>

struct linux_dirent64 {
	unsigned long	d_ino;
	signed long		d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		d_name[];
};

int main() {
    int fd;
    char buf[4096]; // 缓冲区大小
    struct linux_dirent64 *d;
    int nread;
    int bpos;

    // 打开目录
    fd = open(".", O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 循环读取目录项
    while ((nread = syscall(SYS_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (bpos = 0; bpos < nread;) {
            d = (struct linux_dirent64 *)(buf + bpos);
            printf("inode: %llu, name: %s, type: %d off: %ld\n", 
                (unsigned long long)d->d_ino, d->d_name, d->d_type, d->d_off);
            bpos += d->d_reclen;
        }
    }

    if (nread == -1) {
        perror("getdents64");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}