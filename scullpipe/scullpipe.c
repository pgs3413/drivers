#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<asm/current.h>
#include<linux/wait.h>
#include<linux/semaphore.h>
#include<linux/slab.h>
#include<linux/poll.h>
#include<linux/signal.h>

typedef struct {
    char *buf;
    size_t size;
    wait_queue_head_t readq, writeq;
    struct semaphore sem;
    struct fasync_struct *async_queue;
} scull_pipe_t;

dev_t dev = 0;
struct cdev cdev;
scull_pipe_t scull_pipe;

#define BUF_SIZE 4096

int scullpipe_fasync(int fd, struct file *filp, int mode)
{   
    return fasync_helper(fd, filp, mode, &scull_pipe.async_queue);
}

int scullpipe_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int scullpipe_release(struct inode *inode, struct file *filp)
{
    scullpipe_fasync(-1, filp, 0);
    return 0;
}

ssize_t scullpipe_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    if(down_interruptible(&scull_pipe.sem))
        return -ERESTARTSYS;

    while(scull_pipe.size == 0)
    {
        up(&scull_pipe.sem);

        if(filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if(wait_event_interruptible(scull_pipe.readq, (scull_pipe.size != 0)))
            return -ERESTARTSYS;

        if(down_interruptible(&scull_pipe.sem))
            return -ERESTARTSYS;
    }

    if(count > scull_pipe.size)
        count = scull_pipe.size;

    if(copy_to_user(buf, scull_pipe.buf, count))
    {
        up(&scull_pipe.sem);
        return -EFAULT;
    }
    
    scull_pipe.size = 0;
    wake_up_interruptible(&scull_pipe.writeq);
    up(&scull_pipe.sem);

    return count;
}

ssize_t scullpipe_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    if(down_interruptible(&scull_pipe.sem))
        return -ERESTARTSYS;

    while(scull_pipe.size != 0)
    {
        up(&scull_pipe.sem);

        if(filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if(wait_event_interruptible(scull_pipe.writeq, (scull_pipe.size == 0)))
            return -ERESTARTSYS;

        if(down_interruptible(&scull_pipe.sem))
            return -ERESTARTSYS;
    }

    if(count > BUF_SIZE)
        count = BUF_SIZE;

    if(copy_from_user(scull_pipe.buf, buf, count))
    {
        up(&scull_pipe.sem);
        return -EFAULT;
    }
    
    scull_pipe.size = count;
    wake_up_interruptible(&scull_pipe.readq);
    up(&scull_pipe.sem);

    if(scull_pipe.async_queue)
        kill_fasync(&scull_pipe.async_queue, SIGIO, POLL_IN);

    return count;
}

__poll_t scullpipe_poll(struct file *filp, struct poll_table_struct *pt)
{

    __poll_t mask = 0;
    unsigned mode = filp->f_flags & O_ACCMODE;
    unsigned read_mode = mode == O_RDONLY || mode == O_RDWR;
    unsigned write_mode = mode == O_WRONLY || mode == O_RDWR;

    down(&scull_pipe.sem);

    if(read_mode)
        poll_wait(filp, &scull_pipe.readq, pt);
    if(write_mode)
        poll_wait(filp, &scull_pipe.writeq, pt);
    
    if(scull_pipe.size != 0)
        mask |= POLLIN | POLLRDNORM;
    else
        mask |= POLLOUT | POLLWRNORM;
        
    up(&scull_pipe.sem);

    return mask;
}

struct file_operations fops = {
    .open = scullpipe_open,
    .release = scullpipe_release,
    .read = scullpipe_read,
    .write = scullpipe_write,
    .poll = scullpipe_poll,
    .fasync = scullpipe_fasync
};

void scullpipe_exit(void)
{

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);
    if(scull_pipe.buf)
        kfree(scull_pipe.buf);

    printk(KERN_ALERT"scullpipe exit.\n");

}

int scullpipe_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev, 0, 1, "scullpipe");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        scullpipe_exit();
        return -EFAULT;
    }

    scull_pipe.buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    if(!scull_pipe.buf)
    {
        printk(KERN_ALERT"kmalloc buf fail.\n");
        scullpipe_exit();
        return -ENOMEM;
    }
    scull_pipe.size = 0;
    scull_pipe.async_queue = 0;

    sema_init(&scull_pipe.sem, 1);

    init_waitqueue_head(&scull_pipe.readq);
    init_waitqueue_head(&scull_pipe.writeq);

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &fops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
    {
        printk(KERN_ALERT"cdev add fail.\n");
        scullpipe_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"scullpipe init successful.\n");
    return 0;
}


module_init(scullpipe_init);
module_exit(scullpipe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");