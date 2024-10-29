#include<linux/cdev.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<asm/current.h>
#include<linux/sched.h>
#include<linux/spinlock.h>

typedef struct scull_dev_struct {
    char *buf;
    size_t size;
    pid_t key;
    struct scull_dev_struct *next;
} scull_dev;

#define BUF_SIZE 4096

dev_t dev = 0;
struct cdev cdev;

scull_dev *scull_dev_list = NULL;
spinlock_t list_lock = __SPIN_LOCK_UNLOCKED(list_lock);

scull_dev *search_scull_dev(void)
{
    scull_dev *scull_dev_p = NULL, *cur;
    pid_t sid = 0;

    spin_lock(&list_lock);

    cur = scull_dev_list;
    sid = pid_vnr(task_session(current));

    while(cur)
    {
        if(cur->key == sid)
        {
            scull_dev_p = cur;
            break;
        }
        cur = cur->next;
    }

    if(scull_dev_p)
        goto out;

    scull_dev_p = kmalloc(sizeof(scull_dev), GFP_KERNEL);
    if(!scull_dev_p)
        goto out;

    scull_dev_p->buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    if(!scull_dev_p->buf)
    {
        kfree(scull_dev_p);
        scull_dev_p = NULL;
        goto out;
    }

    scull_dev_p->key = sid;
    scull_dev_p->size = 0;
    scull_dev_p->next = scull_dev_list;
    scull_dev_list = scull_dev_p;

    out:
        spin_unlock(&list_lock);
        if(scull_dev_p)
            printk(KERN_ALERT"search scull_dev successful for sid: %d\n", sid);
        else
            printk(KERN_ALERT"search scull_dev fail for sid: %d\n", sid);
        return scull_dev_p;
}

int scullpriv_open(struct inode *inode, struct file *filp)
{
    scull_dev *scull_dev_p = search_scull_dev();
    if(!scull_dev_p)
        return -EFAULT;

    filp->private_data = scull_dev_p;

    if((filp->f_flags & O_ACCMODE) == O_WRONLY)
        scull_dev_p->size = 0;

    return 0;
}

int scullpriv_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t scullpriv_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    scull_dev *scull_dev_p = filp->private_data;

    if(*f_pos >= scull_dev_p->size)
        return 0;

    if(*f_pos + count > scull_dev_p->size)
        count = scull_dev_p->size - *f_pos;

    if(copy_to_user(buf, scull_dev_p->buf + *f_pos, count))
        return -EFAULT;

    *f_pos += count;
    return count;
}

ssize_t scullpriv_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    scull_dev *scull_dev_p = filp->private_data;

    if(*f_pos >= BUF_SIZE)
        return 0;

    if(*f_pos + count > BUF_SIZE)
        count = BUF_SIZE - *f_pos;

    if(copy_from_user(scull_dev_p->buf + *f_pos, buf, count))
        return -EFAULT;

    *f_pos += count;
    if(scull_dev_p->size < *f_pos)
        scull_dev_p->size = *f_pos;

    return count;
}

struct file_operations fops = {
    .open = scullpriv_open,
    .release = scullpriv_release,
    .read = scullpriv_read,
    .write = scullpriv_write
};

void scullpriv_exit(void)
{
    scull_dev *cur = scull_dev_list, *next;
    
    scull_dev_list = NULL;
    while(cur)
    {
        next = cur->next;
        if(cur->buf)
            kfree(cur->buf);
        kfree(cur);
        cur = next;
    }

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);

    printk(KERN_ALERT"scullpriv exit.\n");
}

int scullpriv_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev, 0, 1, "scullpriv");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        scullpriv_exit();
        return -EFAULT;
    }

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &fops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
    {
        printk(KERN_ALERT"cdev add fail.\n");
        scullpriv_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"scullpriv init successful.\n");
    return 0;
}


module_init(scullpriv_init);
module_exit(scullpriv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");