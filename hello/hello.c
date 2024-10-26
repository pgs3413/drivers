#include<linux/module.h>
#include<linux/init.h>
#include<linux/moduleparam.h>

char *whom = "hello\n";
int howmany = 1;

module_param(whom, charp, 0);
module_param(howmany, int, 0);

int hello(void){
    int i = 0;
    for(; i < howmany; i++)
        printk(KERN_ALERT"%s\n", whom);
    return 0;
}

void bye(void){
    printk(KERN_ALERT"good, bye\n");
}

module_init(hello);
module_exit(bye);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("pan");