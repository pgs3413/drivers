#include<linux/module.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<linux/gfp.h>
#include<linux/fs.h>
#include<linux/workqueue.h>
#include<linux/slab.h>
#include<linux/uio.h>

typedef struct {
    int rw;
    struct kiocb *kiocb;
    struct iov_iter *iov_iter;
    struct delayed_work delayed_work;
} aio_work_t;

dev_t dev = 0;
struct cdev cdev;

char *buf = 0;
size_t size = 0;

void aio_work_func(struct work_struct *work)
{
    struct delayed_work *delayed_work = container_of(work, struct delayed_work, work);
    aio_work_t *aio_work = container_of(delayed_work, aio_work_t, delayed_work);
    size_t ret, count;

    if(aio_work->rw == 0)
    {
        ret = copy_to_iter(buf, size, aio_work->iov_iter);
        if (ret != size) {
            printk(KERN_ALERT"copy_to_iter failed\n");
        } 
    }

    if(aio_work->rw == 1)
    {
        count = iov_iter_count(aio_work->iov_iter);
        if(count > PAGE_SIZE)
            count = PAGE_SIZE;
        ret = copy_from_iter(buf, count, aio_work->iov_iter);
        if (ret != count) {
            printk(KERN_ALERT"copy_from_iter failed\n");
        }
        size = ret; 
    }

    aio_work->kiocb->ki_complete(aio_work->kiocb, ret, 0);
    printk(KERN_ALERT"scullaio finished %d\n", aio_work->rw);
    kfree(aio_work);
}

int scullaio_open(struct inode *inode, struct file *filp)
{
    if((filp->f_flags & O_ACCMODE) == O_WRONLY)
        size = 0;

    return 0;
}

int scullaio_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t scullaio_read(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    aio_work_t *aio_work = kmalloc(sizeof(aio_work_t), GFP_KERNEL);
    if(!aio_work)
    {
        printk(KERN_ALERT"kmalloc fail.\n");
        return -ENOMEM;
    }

    aio_work->rw = 0;
    aio_work->kiocb = kiocb;
    aio_work->iov_iter = iov_iter;
    INIT_DELAYED_WORK(&aio_work->delayed_work, aio_work_func);
    schedule_delayed_work(&aio_work->delayed_work, 2 * HZ);

    printk(KERN_ALERT"scullaio_read commited\n");

    return -EIOCBQUEUED;
}

ssize_t scullaio_write(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    aio_work_t *aio_work = kmalloc(sizeof(aio_work_t), GFP_KERNEL);
    if(!aio_work)
    {
        printk(KERN_ALERT"kmalloc fail.\n");
        return -ENOMEM;
    }

    aio_work->rw = 1;
    aio_work->kiocb = kiocb;
    aio_work->iov_iter = iov_iter;
    INIT_DELAYED_WORK(&aio_work->delayed_work, aio_work_func);
    schedule_delayed_work(&aio_work->delayed_work, 2 * HZ);

    printk(KERN_ALERT"scullaio_write commited\n");

    return -EIOCBQUEUED;
}

struct file_operations file_ops = {
    .open = scullaio_open,
    .release = scullaio_release,
    .read_iter = scullaio_read,
    .write_iter = scullaio_write
};

void scullaio_exit(void)
{

    if(buf)
        free_page((unsigned long)buf);

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);

    printk(KERN_ALERT"scullaio exit.\n");
}

int scullaio_init(void)
{
    int ret;

    buf = (char *)__get_free_page(GFP_KERNEL);
    if(!buf)
    {
        printk(KERN_ALERT"__get_free_page fail.\n");
        scullaio_exit();
        return -EFAULT;
    }

    ret = alloc_chrdev_region(&dev, 0, 1, "scullaio");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        scullaio_exit();
        return -EFAULT;
    }

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &file_ops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
    {
        printk(KERN_ALERT"cdev add fail.\n");
        scullaio_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"scullaio init successful.\n");
    return 0;

}


module_init(scullaio_init);
module_exit(scullaio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");