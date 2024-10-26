#include "scull.h"

extern scull_dev *scull_dev_list;

void *seq_start(struct seq_file *m, loff_t *pos)
{
    if(*pos >= SCULL_CNT)
        return NULL;
    return scull_dev_list + *pos;
}

void seq_stop(struct seq_file *m, void *v)
{
    return;
}

void *seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    (*pos)++;
    if(*pos >= SCULL_CNT)
        return NULL;
    return scull_dev_list + *pos;
}

int seq_show(struct seq_file *m, void *v)
{
    scull_dev *scull_dev_p;
    char *buf;

    scull_dev_p = (scull_dev*)v;
    buf = kmalloc(scull_dev_p->size + 1, GFP_KERNEL);
    if(!buf)
    {
        printk(KERN_ALERT"kmalloc seq buf fail.\n");
        return -ENOMEM;
    }

    memcpy(buf, scull_dev_p->buf, scull_dev_p->size);
    buf[scull_dev_p->size] = '\0';

    seq_printf(m, "Device %i : %s\n", (int)(scull_dev_p - scull_dev_list), buf);
    
    kfree(buf);
    
    return 0;
}

struct seq_operations seq_ops = {
    .start = seq_start,
    .stop = seq_stop,
    .next = seq_next,
    .show = seq_show
};

int	proc_open(struct inode *inode, struct file *filp){
    return seq_open(filp, &seq_ops);
}

struct proc_ops proc_ops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

