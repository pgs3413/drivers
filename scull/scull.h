#ifndef SCULL_H_
#define SCULL_H_

#include<linux/cdev.h>

typedef struct {
    char *buf;
    size_t size;
    unsigned char init;
    struct cdev cdev;
} scull_dev;

#define SCULL_CNT 2
#define SCULL_BUF_SIZE (1024 * 1024)
#define MAJOR_DEV 0
#define MINOR_DEV 0
#define SCULL_NAME "scull"

#endif