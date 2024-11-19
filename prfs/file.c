#include "prfs.h"

int prfs_open(struct inode *inode, struct file *filp)
{
    pr_alert("open inode ino: %ld\n", inode->i_ino);
    return 0;
}

int prfs_release(struct inode *inode, struct file *filp)
{
    pr_alert("release.\n");
    return 0;
}

ssize_t prfs_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

ssize_t prfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

struct file_operations file_ops = {
    .open = prfs_open,
    .release = prfs_release,
    .read = prfs_read,
    .write = prfs_write
};

struct inode_operations file_inode_ops = {

};
