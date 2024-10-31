#include<sys/io.h>
#include<stdio.h>

int main(int argc, char *argv[])
{
    int ret;
    unsigned char led = 0;

    ret = ioperm(0x60, 1, 1);
    if(ret < 0)
    {
        perror("err");
        return 1;
    }

    ret = ioperm(0x64, 1, 1);
    if(ret < 0)
    {
        perror("err");
        return 1;
    }

    if(argc > 1)
        led = 0x07;

    while (inb(0x64) & 0x2);
    outb(0xed, 0x60);
    while (inb(0x64) & 0x2);
    outb(led, 0x60);

    ioperm(0x60, 1, 0);
    ioperm(0x64, 1, 0);

    printf("successful!\n");
    return 0;
}