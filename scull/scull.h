#ifndef SCULL_H_
#define SCULL_H_

#include<linux/cdev.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/err.h>
#include<linux/fs.h>
#include<linux/string.h>
#include<linux/semaphore.h>

typedef struct {
    char *buf;
    size_t size;
    unsigned char init;
    struct semaphore sem;
    struct cdev cdev;
} scull_dev;

#define SCULL_CNT 2
#define SCULL_BUF_SIZE (1024 * 1024)
#define MAJOR_DEV 0
#define MINOR_DEV 0
#define SCULL_NAME "scull"

#endif