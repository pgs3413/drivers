#include<linux/module.h>
#include<linux/init.h>
#include<linux/gfp.h>
#include<linux/mm.h>
#include<linux/vmalloc.h>

void page_table(unsigned long addr)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pgd = pgd_offset(current->mm, addr);
    if(pgd_none(*pgd))
    {
        printk(KERN_ALERT"NO pgd entry\n");
        return;
    }

    p4d = p4d_offset(pgd, addr);
    if(p4d_none(*p4d))
    {
        printk(KERN_ALERT"NO p4d entry\n");
        return;
    }

    pud = pud_offset(p4d, addr);
    if(pud_none(*pud))
    {
        printk(KERN_ALERT"NO pud entry\n");
        return;
    }

    pmd = pmd_offset(pud, addr);
    if(pmd_none(*pmd))
    {
        printk(KERN_ALERT"NO pmd entry\n");
        return;
    }

    pte = pte_offset_kernel(pmd, addr);
    if(pte_none(*pte))
    {
        printk(KERN_ALERT"NO pte entry\n");
        return;
    }
    
    printk(KERN_ALERT"pte: %lx\n", pte->pte);
}

unsigned long page_address_cal(struct page *page)
{
    unsigned long index, physical_addr, virtual_addr;

    printk(KERN_ALERT"page kernel virtual addr: %px\n", page_address(page));

    // page_address <=> __va(PFN_PHYS(page_to_pfn(x)))

    index = page - vmemmap; //page_to_pfn(page)
    physical_addr = index << PAGE_SHIFT; // PFN_PHYS(index)
    virtual_addr = physical_addr + PAGE_OFFSET; // __va(physical_addr)

    printk(KERN_ALERT"index: %ld physical_addr: %lx PAGE_OFFSET: %lx virtual_addr: %lx\n",
        index, physical_addr, PAGE_OFFSET, virtual_addr);

    printk(KERN_ALERT"kernel code address: %lx\n", (unsigned long)alloc_pages);

    return virtual_addr;
}

void page_print(char *buf)
{
    int size;
    size = sprintf(buf, "HELLO,WORLD!");
    buf[size] = '\0';
    printk(KERN_ALERT"msg: %s\n", buf);
}

void page_macro(void)
{
    printk(KERN_ALERT"MAX_PHYSMEM_BITS: %d\n", MAX_PHYSMEM_BITS);
    printk(KERN_ALERT"VMEMMAP_START: %lx page size:%ld VMALLOC_SIZE_TB:%ld\n", 
    VMEMMAP_START, sizeof(struct page), VMALLOC_SIZE_TB);
}

int page_init(void)
{
    unsigned long addr;
    struct page *page;
    page = alloc_pages(GFP_KERNEL, 0);
    page_table((unsigned long)page);
    addr = page_address_cal(page);
    page_print((char *)addr);
    page_table(addr);
    page_macro();
    
    __free_pages(page, 0);
    printk(KERN_ALERT"page init successful.\n");
    return 0;
}

void page_exit(void) {}

// char *buf = 0;
// char *vbuf = 0;

// void print(void)
// {
//     printk(KERN_ALERT"%s\n", buf);
//     printk(KERN_ALERT"%s\n", vbuf);
// }

// int page_init(void)
// {
//     int size;

//     buf = (char *)__get_free_pages(GFP_KERNEL, 0);
//     if(!buf)
//     {
//         printk(KERN_ALERT"get free pages fail.\n");
//         return -EFAULT;
//     }

//     vbuf = vmalloc(PAGE_SIZE * 10);
//     if(!vbuf)
//     {
//         printk(KERN_ALERT"vmalloc fail.\n");
//         free_pages((unsigned long)buf, 0);
//         return -EFAULT;
//     }

//     printk(KERN_ALERT"module addr: %px page addr: %px vaddr: %px\n", print, buf, vbuf);

//     size = sprintf(buf, "HELLO WORLD FORM PAGE!");
//     buf[size] = '\0';

//     size = sprintf(vbuf, "HELLO WORLD FROM VMALLOC!");
//     vbuf[size] = '\0';

//     return 0;
// }

// void page_exit(void)
// {
//     print();
//     free_pages((unsigned long)buf, 0);
//     vfree(vbuf);
// }

module_init(page_init);
module_exit(page_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");