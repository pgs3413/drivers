#include"prfs.h"


int pdentry_delete(const struct dentry *dentry)
{
    // pr_alert("delete: %s count: %d\n", dentry->d_name.name, dentry->d_lockref.count);
    return 1;
}

int pdentry_revalidate(struct dentry *dentry, unsigned int flags)
{
    pr_alert("revalidate: %s count: %d\n", dentry->d_name.name, dentry->d_lockref.count);
    return 1;
}

struct dentry_operations dentry_ops = {
    .d_delete = pdentry_delete,
    // .d_revalidate = pdentry_revalidate
};

struct dentry *show_dentry;
struct inode *show_inode;

static int dentry_show(struct seq_file *m, void *v) 
{
    if(show_dentry)
        seq_printf(m, "dentry name: %s count: %d inode: %px\n", 
            show_dentry->d_name.name, show_dentry->d_lockref.count, show_dentry->d_inode);
    if(show_inode)
        seq_printf(m, "inode address: %px ino: %ld, nrlink: %d, count: %d\n",
            show_inode, show_inode->i_ino, show_inode->__i_nlink, atomic_read(&show_inode->i_count));
    return 0;
}

static int prfs_proc_open(struct inode *inode, struct file *file) 
{
    return single_open(file, dentry_show, NULL);
}

const struct proc_ops prfs_proc_ops = {
    .proc_open = prfs_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};