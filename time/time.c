#include<linux/module.h>
#include<linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include<linux/jiffies.h>
#include<asm/msr.h>

unsigned long last;

static int time_show(struct seq_file *m, void *v) 
{
    unsigned long now, diff, diff_ms;
    unsigned long ini, end;
    unsigned long start_tsc, end_tsc, diff_tsc, one_second, clock_fr;

    now = jiffies;
    diff = now - last;
    diff_ms = diff * 1000 / HZ;
    seq_printf(m, "HZ: %d jiffies: %lx past: %ld.%lds\n", HZ, now, diff_ms / 1000, diff_ms % 1000);
    last = now;

    ini = rdtsc();
    end = rdtsc();
    seq_printf(m, "the cpu clock between two times of reading tsc : %ld\n", end - ini);

    one_second = jiffies + HZ;
    start_tsc = rdtsc();
    while (jiffies < one_second) ;
    end_tsc = rdtsc();
    diff_tsc = end_tsc - start_tsc;
    clock_fr = diff_tsc / (1000 * 1000 * 100);
    seq_printf(m, "cpu clock frequency: %ld.%ldGHz\n", clock_fr / 10, clock_fr % 10);

    return 0;
}

static int time_open(struct inode *inode, struct file *file) 
{
    return single_open(file, time_show, NULL);
}

static const struct proc_ops ops = {
    .proc_open = time_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

int time_init(void)
{
    last = jiffies;
    proc_create("time", 0, NULL, &ops);
    printk(KERN_ALERT"/proc/time init successful\n");
    return 0;
}

void time_exit(void)
{
    remove_proc_entry("time", NULL);
}

module_init(time_init);
module_exit(time_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");