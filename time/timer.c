#include "time.h"

#define BUF_SIZE 4096
#define TIMER_LOOPS 4
#define TIMER_DELAY 10

typedef struct
{
    unsigned long last_jiffies;
    char *buf;
    size_t size;
    size_t loops;
    wait_queue_head_t wq;
    struct timer_list timer; 
} timer_data;

static void	timer_func(struct timer_list *timer)
{
    timer_data *data = container_of(timer, timer_data, timer);
    unsigned long now = jiffies;
    unsigned long diff = now - data->last_jiffies;
    data->last_jiffies = now;

    data->size += sprintf(data->buf + data->size, "%-12ld%-10ld%-10ld%-10d%-10d%-10s\n",
    data->last_jiffies, diff, in_interrupt(), current->pid, smp_processor_id(), current->comm);

    data->loops--;
    if(data->loops)
        mod_timer(&data->timer, data->last_jiffies + TIMER_DELAY);
     else 
        wake_up(&data->wq); 
    
}

static int timer_show(struct seq_file *m, void *v) 
{
    timer_data *data = kmalloc(sizeof(timer_data), GFP_KERNEL);
    if(!data)
    {
        seq_printf(m, "kmalloc data fail\n");
        return 0;
    }

    data->buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    if(!data->buf)
    {
        kfree(data);
        seq_printf(m, "kmalloc buf fail\n");
        return 0;
    }

    data->size = 0;
    data->loops = TIMER_LOOPS;
    data->last_jiffies = jiffies;
    init_waitqueue_head(&data->wq);
    timer_setup(&data->timer, timer_func, 0);

    data->size += sprintf(data->buf + data->size, "%-12s%-10s%-10s%-10s%-10s%-10s\n", 
    "time", "delta", "inirq", "pid", "cpu", "command");
    data->size += sprintf(data->buf + data->size, "%-12ld%-10d%-10ld%-10d%-10d%-10s\n",
    data->last_jiffies, 0, in_interrupt(), current->pid, smp_processor_id(), current->comm);

    mod_timer(&data->timer, data->last_jiffies + TIMER_DELAY);

    wait_event(data->wq, (data->loops == 0));

    data->buf[data->size] = '\0';
    seq_printf(m, "%s", data->buf);

    kfree(data->buf);
    kfree(data);

    return 0;
}

static int timer_open(struct inode *inode, struct file *file) 
{
    return single_open(file, timer_show, NULL);
}

const struct proc_ops timer_ops = {
    .proc_open = timer_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};