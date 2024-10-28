#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<asm/current.h>
#include<linux/wait.h>
#include<linux/sched.h>

wait_queue_head_t wq;
dev_t dev = 0;
struct cdev cdev;
int flag = 0;
struct task_struct *task = 0;

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

    // 1.自动休眠 等待队列与循环判断条件
    // wait_event_interruptible(wq, (flag != 0)); 

    //2.手动休眠 等待对列与条件
    // wait_queue_entry_t entry;
    // init_wait_entry(&entry, 0);
    // prepare_to_wait(&wq, &entry, TASK_INTERRUPTIBLE);
    // if(flag == 0)
    //     schedule();
    // finish_wait(&wq, &entry);

    //3.不使用等待队列
    task = current;
    current->__state = TASK_INTERRUPTIBLE;
    if(flag == 0)
        schedule();
    current->__state = TASK_RUNNING;
    task = 0;

    if(signal_pending(current))
    {
        printk(KERN_ALERT"process %i (%s) awakes by signal\n", current->pid, current->comm);
        return 0;
    }
    flag = 0;
    printk(KERN_ALERT"process %i (%s) awakes\n", current->pid, current->comm);
    return 0;
}

ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_ALERT"process %i (%s) awakening the readers...\n", current->pid, current->comm);
    flag = 1;

    // 1 2都用此方法唤醒
    // wake_up_interruptible(&wq);

    // 唤醒3
    if(task)
        wake_up_process(task);

    return count;
}

struct file_operations fops = {
    .open = open,
    .release = release,
    .read = read,
    .write = write
};

void sleepy_exit(void)
{

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);

}

int sleepy_init(void)
{
    int ret;

    init_waitqueue_head(&wq);

    ret = alloc_chrdev_region(&dev, 0, 1, "sleepy");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        sleepy_exit();
        return -EFAULT;
    }

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &fops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
        {
            printk(KERN_ALERT"cdev add fail.\n");
            sleepy_exit();
            return -EFAULT;
        }

    printk(KERN_ALERT"sleepy init successful.\n");
    return 0;
}


module_init(sleepy_init);
module_exit(sleepy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");