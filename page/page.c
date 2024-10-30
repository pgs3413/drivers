#include<linux/module.h>
#include<linux/init.h>
#include<linux/gfp.h>
#include<linux/mm.h>

int page_init(void)
{
    struct page *page;
    unsigned long index, physical_addr, virtual_addr;

    page = alloc_pages(GFP_KERNEL, 0);

    printk(KERN_ALERT"page kernel virtual addr: %px\n", page_address(page));

    // page_address <=> __va(PFN_PHYS(page_to_pfn(x)))

    index = page - vmemmap; //page_to_pfn(page)
    physical_addr = index << PAGE_SHIFT; // PFN_PHYS(index)
    virtual_addr = physical_addr + PAGE_OFFSET; // __va(physical_addr)

    printk(KERN_ALERT"index: %ld physical_addr: %lx PAGE_OFFSET: %lx virtual_addr: %lx\n",
        index, physical_addr, PAGE_OFFSET, virtual_addr);

    __free_pages(page, 0);
    return 0;
}

void page_exit(void)
{
}

module_init(page_init);
module_exit(page_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");