#include"prfs.h"


int pdentry_delete(const struct dentry *dentry)
{
    pr_alert("delete: %s count: %d\n", dentry->d_name.name, dentry->d_lockref.count);
    return 0;
}

int pdentry_revalidate(struct dentry *dentry, unsigned int flags)
{
    pr_alert("revalidate: %s count: %d\n", dentry->d_name.name, dentry->d_lockref.count);
    return 1;
}

struct dentry_operations dentry_ops = {
    .d_delete = pdentry_delete,
    .d_revalidate = pdentry_revalidate
};

struct dentry *show_dentry;

static int dentry_show(struct seq_file *m, void *v) 
{

    seq_printf(m, "normal count: %d name: %s\n", show_dentry->d_lockref.count, show_dentry->d_name.name);
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