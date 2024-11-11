#include<linux/module.h>
#include<linux/init.h>
#include<linux/blkdev.h>
#include<linux/genhd.h>
#include<linux/gfp.h>
#include<linux/page-flags.h>
#include<linux/blk-mq.h>

int dev_major = 0;
struct gendisk *disk = 0;
int sector_nr = 8;
char *buf = 0;
struct blk_mq_tag_set set;

/**
 * 当gendisk定义了这一个方法，submit_bio就会跳过request_queue，而是
 * 会同步调用这个方法处理单个bio。
 */
blk_qc_t sbull_submit_bio(struct bio *bio)
{
    int write = bio_data_dir(bio);
    struct bio_vec bvec;
    struct bvec_iter iter;
    char *cache_mem, *dev_mem;

    printk(KERN_ALERT"start a bio. diretion: %s task: %s address_space: %px\n", 
        write ? "write" : "read", current->comm, disk->part0->bd_inode->i_mapping);
    bio_for_each_segment(bvec, bio, iter)
    {
        printk(KERN_ALERT"idx: %d sector: %lld offset: %d size: %d mapping: %px\n", 
            iter.bi_idx, iter.bi_sector, bvec.bv_offset, bvec.bv_len, bvec.bv_page->mapping);

        dev_mem = buf + iter.bi_sector * 512;
        cache_mem = page_address(bvec.bv_page) + bvec.bv_offset;
        
        if(write)
            memcpy(dev_mem, cache_mem, bvec.bv_len);
        else
            memcpy(cache_mem, dev_mem, bvec.bv_len);
    }
    
    // bio_endio(bio);

    printk(KERN_ALERT"a bio done.\n");
    return BLK_STS_OK;
}

struct block_device_operations bd_ops = {
    // .submit_bio = sbull_submit_bio
};

blk_status_t sbull_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *data)
{
    struct request *req = data->rq;
    struct bio *bio;

    pr_alert("start a request\n");

    if(blk_rq_is_passthrough(req))
    {
        pr_alert("req is not a fs request\n");
        blk_mq_end_request(req, BLK_STS_IOERR);
        return BLK_STS_IOERR;
    }

    __rq_for_each_bio(bio, req)
        sbull_submit_bio(bio);

    blk_mq_end_request(req, BLK_STS_OK);
    
    pr_alert("a request done\n");
    return BLK_STS_OK;
}

struct blk_mq_ops mq_ops = {
    .queue_rq = sbull_queue_rq
};

void sbull_exit(void)
{

    if(disk)
    {
        del_gendisk(disk); /*反向操作add_disk*/
        blk_cleanup_disk(disk); /*反向操作alloc_disk*/
    }

    blk_mq_free_tag_set(&set);   
        
    /*返还主设备号*/
    if(dev_major > 0)
        unregister_blkdev(dev_major, "sbull");

    if(buf)
        vfree(buf);

    printk(KERN_ALERT"sbull exit.\n");
}

int sbull_init(void)
{
    int ret;

    buf = vzalloc(sector_nr * 512);
    if(!buf)
    {
        printk(KERN_ALERT"vmalloc failed.\n");
        sbull_exit();
        return -EFAULT;
    }

    /*分配了一个块主设备号*/
    dev_major = register_blkdev(dev_major, "sbull");
    if(dev_major <= 0)
    {
        printk(KERN_ALERT"register_blkdev failed.\n");
        sbull_exit();
        return -EFAULT;
    }

    /*
        分配并初始化了一个request_queue,gendisk,bdev_inode(block_device,inode)
        gendisk->queue = request_queue
        gendish->part0 = block_device
        block_device->bd_disk = gendisk
        block_device->bd_inode = inode
        把part0加入到分区表中
    */
    disk = blk_alloc_disk(NUMA_NO_NODE);
    if(!disk)
    {
        printk(KERN_ALERT"blk_alloc_disk failed.\n");
        sbull_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"disk alloc successful. major: %d\n", dev_major);

    disk->major = dev_major;
    disk->first_minor = 0;
    disk->minors = 1;
    disk->fops = &bd_ops;
    snprintf(disk->disk_name, DISK_NAME_LEN, "sbull");
    set_capacity(disk, sector_nr);

    /*
        构建目mq所需的tag_set
    */
    ret = blk_mq_alloc_sq_tag_set(&set, &mq_ops, 128, 0);
    if(ret)
    {
        printk(KERN_ALERT"blk_mq_alloc_sq_tag_set failed.\n");
        sbull_exit();
        return -EFAULT;
    }

    /*
        对disk->queue进行初始化：分配软件队列、硬件队列等
    */
    ret = blk_mq_init_allocated_queue(&set, disk->queue);
    if(ret)
    {
        printk(KERN_ALERT"blk_mq_alloc_sq_tag_set failed.\n");
        sbull_exit();
        return -EFAULT;
    }

    printk(KERN_ALERT"ready to add disk.\n");
    /*
        device_add_disk: add disk information to kernel list
            device_add: 可在sysfs看到该block_device
            bdev_add: 把bdev->bd_inode加入到一个hashtable中，如此便可通过设备号找到inode->block_device->gendisk
            disk_scan_partitions: 扫描分区表
                : 根据不同的格式，读取不同的扇区的分区表，自动构造分区信息
    */
    add_disk(disk); 

    printk(KERN_ALERT"sbull init successful.\n");
    return 0;
}

module_init(sbull_init);
module_exit(sbull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");