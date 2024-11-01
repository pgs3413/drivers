#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<asm/io.h>
#include<linux/ioport.h>
#include<linux/interrupt.h>
#include<linux/wait.h>
#include<linux/atomic.h>

int data_port = 0x60;
int cmd_port = 0x64;
unsigned char led_status = 0;
wait_queue_head_t wq;
atomic_t flag;

// static void write_64(unsigned char cmd)
// {
//     unsigned char cant_write = 0x02;
//     while(inb(cmd_port) & cant_write);
//     outb(cmd, cmd_port);
// }

static void write_60(unsigned char cmd)
{
    unsigned char cant_write = 0x02;
    while(inb(cmd_port) & cant_write);
    outb(cmd, data_port);
}

static unsigned char read_60(void)
{
    unsigned char can_read = 0x01;
    while(!(inb(cmd_port) & can_read));
    return inb(data_port);
}

// static unsigned char read_command(void)
// {
//     write_64(0x20);
//     return read_60();
// }

// static unsigned char read_selfcheck(void)
// {
//     write_64(0xAA);
//     return read_60();
// }

// static unsigned char read_echo(void)
// {
//     write_60(0xEE);
//     return read_60();
// }

static unsigned char set_led(unsigned char led_status)
{
    unsigned char response;

    write_60(0xED);

    response = read_60();
    if(response != 0xFA)
        return response;

    write_60(led_status);

    response = read_60();
    if(response != 0xFA)
        return response;
    
    write_60(0x80);
    
    return 0;
}

static int kb_show(struct seq_file *m, void *v) 
{
    unsigned char led_response;
    // seq_printf(m, "command: 0x%x selfcheck: 0x%x echo: 0x%x\n", read_command(), read_selfcheck(), read_echo());
    if(led_status)
        led_status = 0;
    else 
        led_status = 0x07;
    // led_response = set_led(led_status);
    // if(led_response)
    //     seq_printf(m, "set led fail.\n");
    // else
    //     seq_printf(m, "set led successful.\n");

    atomic_set(&flag, 0);
    wait_event_interruptible(wq, (atomic_read(&flag) != 0));
    atomic_set(&flag, 0);

    seq_printf(m, "good!\n");

    return 0;
}

static int kb_open(struct inode *inode, struct file *file) 
{
    return single_open(file, kb_show, NULL);
}

const struct proc_ops kb_ops = {
    .proc_open = kb_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

irqreturn_t kb_irq_handler(int irq, void *wq)
{
    printk(KERN_ALERT"receive a interrupt irq: %d\n", irq);
    atomic_set(&flag, 1);
    wake_up_interruptible(wq);
    return IRQ_NONE;
}

int kb_init(void)
{
    struct resource *resource;
    int ret;

    resource = request_region(0x60, 1, "kb");
    if(!resource)
        printk(KERN_ALERT"can`t get I/O address 0x60\n");

    ret = request_irq(1, kb_irq_handler, IRQF_SHARED, "kb", &wq);
    if(ret)
        printk(KERN_ALERT"can`t request irq 1\n");
    else
        printk(KERN_ALERT"request irq 1 successful.\n");

    init_waitqueue_head(&wq);
    atomic_set(&flag, 0);

    proc_create("kb", 0, NULL, &kb_ops);
    printk(KERN_ALERT"kb init successful\n");
    return 0;
}

void kb_exit(void)
{
    free_irq(1, &wq);
    remove_proc_entry("kb", NULL);
    printk(KERN_ALERT"kb exit.\n");
}

module_init(kb_init);
module_exit(kb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");