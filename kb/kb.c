#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<asm/io.h>

int data_port = 0x60;
int cmd_port = 0x64;
unsigned char led_status = 0;

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
    led_response = set_led(led_status);
    if(led_response)
        seq_printf(m, "set led fail.\n");
    else
        seq_printf(m, "set led successful.\n");
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

int kb_init(void)
{
    proc_create("kb", 0, NULL, &kb_ops);
    printk(KERN_ALERT"kb init successful\n");
    return 0;
}

void kb_exit(void)
{
    remove_proc_entry("kb", NULL);
}

module_init(kb_init);
module_exit(kb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");