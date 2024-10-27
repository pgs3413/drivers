#include<linux/module.h>
#include<linux/init.h>
#include<linux/completion.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<asm/current.h>

struct completion com;
dev_t dev = 0;
struct cdev cdev;

int open(struct inode *inode, struct file *filp)
{
    return 0;
}

int release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_ALERT"process %i (%s) going to sleep\n", current->pid, current->comm);
    wait_for_completion(&com);
    printk(KERN_ALERT"process %i (%s) awakes\n", current->pid, current->comm);
    return 0;
}

ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_ALERT"process %i (%s) awakening the readers...\n", current->pid, current->comm);
    complete(&com);
    return count;
}

struct file_operations fops = {
    .open = open,
    .release = release,
    .read = read,
    .write = write
};

void complete_exit(void)
{

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);

}

int complete_init(void)
{
    int ret;

    init_completion(&com);

    ret = alloc_chrdev_region(&dev, 0, 1, "complete");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        complete_exit();
        return -EFAULT;
    }

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &fops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
        {
            printk(KERN_ALERT"cdev add fail.\n");
            complete_exit();
            return -EFAULT;
        }

    printk(KERN_ALERT"complete init successful.\n");
    return 0;
}


module_init(complete_init);
module_exit(complete_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");