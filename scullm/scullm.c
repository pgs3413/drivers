#include<linux/cdev.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/mm.h>

dev_t dev = 0;
struct cdev cdev;
struct page *page0 = 0;
struct page *page1 = 0;

int scullm_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int scullm_release(struct inode *inode, struct file *filp)
{
    return 0;
}

vm_fault_t scullm_fault(struct vm_fault *vmf)
{
    printk(KERN_ALERT"scullm_fault called\n");
    get_page(page1);
    vmf->page = page1;
    return 0;
}

struct vm_operations_struct vm_ops = {
    .fault = scullm_fault
};

int scullm_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long ret;
    unsigned long pfn;
    unsigned long size = vma->vm_end - vma->vm_start;

    if(size > 4096)
        return -EINVAL;

    if(vma->vm_pgoff > 1)
        return -EINVAL;

    if(vma->vm_pgoff == 0)
    {
        pfn = page_to_pfn(page0);
        ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
        if(ret)
            return -EAGAIN;
    }

    if(vma->vm_pgoff == 1)
    {
        vma->vm_ops = &vm_ops;
    }

    return 0;
}


struct file_operations fops = {
    .open = scullm_open,
    .release = scullm_release,
    .mmap = scullm_mmap
};

void scullm_exit(void)
{

    cdev_del(&cdev);
    if(dev)
        unregister_chrdev_region(dev, 1);

    if(page0)
        __free_pages(page0, 0);

    if(page1)
        __free_pages(page1, 0);

    printk(KERN_ALERT"scullm exit.\n");
}

int scullm_init(void)
{
    int ret;

    page0 = alloc_pages(GFP_KERNEL, 0);
    if(!page0)
    {
        scullm_exit();
        return -EFAULT;
    }

    page1 = alloc_pages(GFP_KERNEL, 0);
    if(!page1)
    {
        scullm_exit();
        return -EFAULT;
    }

    ret = alloc_chrdev_region(&dev, 0, 1, "scullm");
    if(ret)
    {
        printk(KERN_ALERT"alloc chrdev region fail.\n");
        scullm_exit();
        return -EFAULT;
    }

    cdev_init(&cdev, NULL);
    cdev.owner = THIS_MODULE;
    cdev.ops = &fops;
    ret = cdev_add(&cdev, dev, 1);
    if(ret)
    {
        printk(KERN_ALERT"cdev add fail.\n");
        scullm_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"scullm init successful.\n");
    return 0;
}


module_init(scullm_init);
module_exit(scullm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");