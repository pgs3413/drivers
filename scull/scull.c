#include "scull.h"
#include "ioc.h"

scull_dev *scull_dev_list = 0;
dev_t dev = 0;
int major_dev = MAJOR_DEV;
extern struct proc_ops proc_ops;

int scull_open(struct inode *inode, struct file *filp)
{
    scull_dev *scull_dev_p;
    scull_dev_p = container_of(inode->i_cdev, scull_dev, cdev);
    filp->private_data = scull_dev_p;

    if((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        if(down_interruptible(&scull_dev_p->sem))
            return -ERESTARTSYS;
        scull_dev_p->size = 0;
        up(&scull_dev_p->sem);
    }
        
    return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    scull_dev *scull_dev_p = filp->private_data;

    if(down_interruptible(&scull_dev_p->sem))
        return -ERESTARTSYS;

    if(*f_pos >= scull_dev_p->size)
    {
        up(&scull_dev_p->sem);
        return 0;
    }

    if(*f_pos + count > scull_dev_p->size)
        count = scull_dev_p->size - *f_pos;

    if(copy_to_user(buf, scull_dev_p->buf + *f_pos, count))
    {
        up(&scull_dev_p->sem);
        return -EFAULT;
    }

    *f_pos += count;

    up(&scull_dev_p->sem);
    return count;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    scull_dev *scull_dev_p;
    scull_dev_p = filp->private_data;

    if(down_interruptible(&scull_dev_p->sem))
        return -ERESTARTSYS;

    if(*f_pos >= SCULL_BUF_SIZE)
    {
        up(&scull_dev_p->sem);
        return 0;
    }

    if(*f_pos + count > SCULL_BUF_SIZE)
        count = SCULL_BUF_SIZE - *f_pos;

    if(copy_from_user(scull_dev_p->buf + *f_pos, buf, count))
    {
        up(&scull_dev_p->sem);
        return -EFAULT;
    }

    *f_pos += count;
    if(scull_dev_p->size < *f_pos)
        scull_dev_p->size = *f_pos;

    up(&scull_dev_p->sem);
    return count;
}

long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    size_t *sizep;
    scull_dev *scull_dev_p;
    scull_dev_p = filp->private_data;

    switch(cmd)
    {
        case SCULL_CLEAR:

            if(!capable(CAP_SYS_ADMIN))
                return -EPERM;

            if(down_interruptible(&scull_dev_p->sem))
                return -ERESTARTSYS;
            scull_dev_p->size = 0;
            up(&scull_dev_p->sem);           
            return 0;

        case SCULL_GETSIZE:

            sizep = (size_t *)arg;

            if(!access_ok(sizep, sizeof(size_t)))
                return -EFAULT;

            if(down_interruptible(&scull_dev_p->sem))
                return -ERESTARTSYS;
            __put_user(scull_dev_p->size, sizep);
            up(&scull_dev_p->sem);           
            return 0;

        default:
            return -ENOTTY;        
    }

    return 0;
}

const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl
};

void scull_exit(void);

int scull_init(void)
{

    int i, ret;
    dev_t num;

    scull_dev_list = kmalloc(SCULL_CNT * sizeof(scull_dev), GFP_KERNEL);
    if(!scull_dev_list)
    {
        printk(KERN_ALERT"kmalloc scull_dev_list fail.\n");
        return -ENOMEM;
    }
    memset(scull_dev_list, 0, SCULL_CNT * sizeof(scull_dev));

    for(i = 0; i < SCULL_CNT; i++)
    {
        scull_dev_list[i].buf = kmalloc(SCULL_BUF_SIZE, GFP_KERNEL);
        if(!scull_dev_list[i].buf)
        {
            printk(KERN_ALERT"kmalloc scull_buf fail.\n");
            scull_exit();
            return -ENOMEM;
        }
        sema_init(&scull_dev_list[i].sem, 1);
    }

    ret = alloc_chrdev_region(&dev, MINOR_DEV, SCULL_CNT, SCULL_NAME);
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        scull_exit();
        return -EFAULT;
    }
    major_dev = MAJOR(dev);

    for(i = 0; i < SCULL_CNT; i++)
    {
        cdev_init(&scull_dev_list[i].cdev, NULL);
        scull_dev_list[i].cdev.owner = THIS_MODULE;
        scull_dev_list[i].cdev.ops = &fops;
        num = MKDEV(major_dev, MINOR_DEV + i);
        ret = cdev_add(&scull_dev_list[i].cdev, num, 1);
        if(ret)
        {
            printk(KERN_ALERT"cdev add fail.\n");
            scull_exit();
            return -EFAULT;
        }
        scull_dev_list[i].init = 1;
    }

    proc_create(SCULL_NAME, 0, NULL, &proc_ops);
    printk(KERN_ALERT"scull init successful.\n");
    return 0;
}

void scull_exit(void){

    int i;

    if(scull_dev_list)
    {

        for(i = 0; i < SCULL_CNT; i++)
        {
            if(scull_dev_list[i].init)
                cdev_del(&scull_dev_list[i].cdev);

            if(scull_dev_list[i].buf)
                kfree(scull_dev_list[i].buf);
        }

        if(dev)
            unregister_chrdev_region(dev, SCULL_CNT);

        kfree(scull_dev_list);
        remove_proc_entry(SCULL_NAME, NULL);
    }   

    printk(KERN_ALERT"scull exit.\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");