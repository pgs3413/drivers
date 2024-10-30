#include "time.h"

#define BUF_SIZE 4096
#define TIMER_LOOPS 10

typedef struct
{
    unsigned long last_jiffies;
    char *buf;
    size_t size;
    size_t loops;
    wait_queue_head_t wq;
    // struct tasklet_struct tasklet;
    struct work_struct work; 
} tasklet_data;

// static void	tasklet_func(unsigned long arg)
// {
//     tasklet_data *data = (tasklet_data*)arg;
//     unsigned long now = jiffies;
//     unsigned long diff = now - data->last_jiffies;
//     data->last_jiffies = now;

//     data->size += sprintf(data->buf + data->size, "%-12ld%-10ld%-10ld%-10d%-10d%-10s\n",
//     data->last_jiffies, diff, in_interrupt(), current->pid, smp_processor_id(), current->comm);

//     data->loops--;
//     if(data->loops)
//         tasklet_schedule(&data->tasklet);
//      else 
//         wake_up(&data->wq); 
// }

static void	work_func(struct work_struct *work)
{
    tasklet_data *data = container_of(work, tasklet_data, work);
    unsigned long now = jiffies;
    unsigned long diff = now - data->last_jiffies;
    data->last_jiffies = now;

    data->size += sprintf(data->buf + data->size, "%-12ld%-10ld%-10ld%-10d%-10d%-10s\n",
    data->last_jiffies, diff, in_interrupt(), current->pid, smp_processor_id(), current->comm);

    data->loops--;
    if(data->loops)
        schedule_work(&data->work);
     else 
        wake_up(&data->wq); 
}

static int tasklet_show(struct seq_file *m, void *v) 
{
    tasklet_data *data = kmalloc(sizeof(tasklet_data), GFP_KERNEL);
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
    // tasklet_init(&data->tasklet, tasklet_func, (unsigned long)data);
    INIT_WORK(&data->work, work_func);

    data->size += sprintf(data->buf + data->size, "%-12s%-10s%-10s%-10s%-10s%-10s\n", 
    "time", "delta", "inirq", "pid", "cpu", "command");
    data->size += sprintf(data->buf + data->size, "%-12ld%-10d%-10ld%-10d%-10d%-10s\n",
    data->last_jiffies, 0, in_interrupt(), current->pid, smp_processor_id(), current->comm);

    // tasklet_schedule(&data->tasklet);
    schedule_work(&data->work);

    wait_event(data->wq, (data->loops == 0));

    data->buf[data->size] = '\0';
    seq_printf(m, "%s", data->buf);

    kfree(data->buf);
    kfree(data);

    return 0;
}

static int tasklet_open(struct inode *inode, struct file *file) 
{
    return single_open(file, tasklet_show, NULL);
}

const struct proc_ops tasklet_ops = {
    .proc_open = tasklet_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};