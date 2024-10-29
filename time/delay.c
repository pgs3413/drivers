#include "time.h"

static int delay_show(struct seq_file *m, void *v) 
{
    unsigned long start, end;

    start = jiffies;
    set_current_state(TASK_UNINTERRUPTIBLE);
    schedule_timeout(HZ);
    end = jiffies;

    seq_printf(m, "%ld  %ld\n", start, end);

    return 0;
}

static int delay_open(struct inode *inode, struct file *file) 
{
    return single_open(file, delay_show, NULL);
}

const struct proc_ops delay_ops = {
    .proc_open = delay_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};