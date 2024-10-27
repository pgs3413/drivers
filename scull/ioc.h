#ifndef SCULL_IOC_H
#define SCULL_IOC_H

#include<linux/ioctl.h>

#define SCULL_IOC_TYPE 'P'

#define SCULL_GETSIZE   _IO(SCULL_IOC_TYPE, 0)
#define SCULL_CLEAR     _IO(SCULL_IOC_TYPE, 1)

#endif