#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs_context.h>
#include<linux/magic.h>
#include "prfs.h"

struct super_operations sb_ops = {
};

/*
    创建文件系统根inode
*/
struct inode *get_root_inode(struct super_block *sb, umode_t mode)
{
    // 分配并初始化一个inode；将该inode加入到sb->s_inodes中
    struct inode *inode = new_inode(sb);
    if(!inode)
        return NULL;

    inode->i_ino = 1; //初始化为root inode
    inode_init_owner(&init_user_ns, inode, NULL, mode);//设置uid、gid、mode
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode); //设置时间
    inode->i_size = 4096;
    //跟目录相关的操作
    inode->i_op = &dir_inode_ops;
	inode->i_fop = &dir_file_ops;
    //增加硬链接树，因为"."
    inc_nlink(inode);

    return inode;
}

/*
    初始化super_block
    创建root inode以及root dentry
*/
int prfs_fill_super(struct super_block *sb, struct fs_context *fc)
{
    struct inode *inode;
    umode_t mode = 0777;
    int ret;

    sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= RAMFS_MAGIC;
	sb->s_op		= &sb_ops; // super block operations
	sb->s_time_gran		= 1;

    ret = disk_init(mode, "root.txt", "this is a root file.\n");
    if(ret)
    {
        pr_alert("disk_init failed.\n");
        return -ENOMEM;
    }   

    inode = get_root_inode(sb, mode | S_IFDIR);
    /*
        创建root dentry:
            分配一个dentry对象
            路径名为 "/"
            dentry->d_parent = dentry; dentry->d_sb = inode->i_sb; dentry->d_op = dentry->d_sb->s_d_op
            dentry->d_inode = inode;
    */
    sb->s_root = d_make_root(inode);
    if(!sb->s_root)
    {
        pr_alert("prfs_fill_super failed.\n");
        disk_destroy();
        return -ENOMEM;
    }

    // pr_alert("prfs_fill_super successful.\n");
    return 0;
}

/*
    内核会调用此方法来获得跟被本次安装相关的super_block、root inode、root dentry
    需要设置:
        fc->root为root dentry
        fc->root->d_sb为super block
    内核最后通过调用do_new_mount_fc完成vfsmount的创建以及注册
*/
int prfs_get_tree(struct fs_context *fc)
{
    /*
        遍历file_system_type->fs_supers里所有的super_block：（vfs_get_single_super意味着使用第一个）
            1.如果找到，则使用旧的super_block：
                增加active计数器
                fc->root = sb->s_root
            2.如果没有找到，则分配并初始化一个新的super_block: 
                active计数器为1
                sb->s_fs_info = fc->s_fs_info
                加入到file_system_type->fs_supers中
                调用fill_super进行完整初始化
                fc->root = sb->s_root
    */
    return vfs_get_super(fc, vfs_get_single_super, prfs_fill_super);
}

struct fs_context_operations fc_ops = {
    .get_tree = prfs_get_tree
};

/*
    fs_context用于描述安装(mount)过程时的全部相关信息及操作（安装上下文）
    当mount一个文件系统时，内核自动创建一个fs_context并调用此方法进行初始化
    需要初始化：
        fc->s_fs_info 私有数据，最后赋值给sb->s_fs_info
        fc->ops 相关操作
*/
int prfs_init_fs_context(struct fs_context *fc)
{
    fc->ops = &fc_ops;
    return 0;
}

/* 
    卸载会减少active计数器
    当active计数器为0时，调用此方法
*/
void prfs_kill_sb(struct super_block *sb)
{
    disk_destroy();
    kill_litter_super(sb);
    // pr_alert("prfs_kill_sb successful.\n");
}

struct file_system_type prfs_type = {
    .name = "prfs",
    .owner = THIS_MODULE,
    .init_fs_context = prfs_init_fs_context,
    .kill_sb = prfs_kill_sb
};

void prfs_exit(void)
{
    unregister_filesystem(&prfs_type);
    remove_proc_entry("pdentry", NULL);
    // pr_alert("prfs exit.\n");
}

int prfs_init(void)
{
    int ret = register_filesystem(&prfs_type);
    if(ret)
    {
        pr_alert("register_filesystem failed.\n");
        return -EFAULT;
    }

    proc_create("pdentry", 0, NULL, &prfs_proc_ops);
    // pr_alert("prfs init sucessful.\n");
    return 0;
}

module_init(prfs_init);
module_exit(prfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");
MODULE_DESCRIPTION("pan ram file system");