#include "time.h"

extern struct proc_ops info_ops;
extern struct proc_ops delay_ops;
extern struct proc_ops timer_ops;
extern unsigned long last;

int time_init(void)
{
    last = jiffies;
    proc_create("timeinfo", 0, NULL, &info_ops);
    proc_create("delay", 0, NULL, &delay_ops);
    proc_create("timer", 0, NULL, &timer_ops);
    printk(KERN_ALERT"time init successful\n");
    return 0;
}

void time_exit(void)
{
    remove_proc_entry("timeinfo", NULL);
    remove_proc_entry("delay", NULL);
    remove_proc_entry("timer", NULL);
}

module_init(time_init);
module_exit(time_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");