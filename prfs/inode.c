#include "prfs.h"

struct inode *get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, unsigned short ino, unsigned long size)
{
    struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = ino;
		inode_init_owner(&init_user_ns, inode, dir, mode);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        inode->i_size = size;
		switch (mode & S_IFMT) {
		default:
            // pr_alert("create a new unknown inode.\n");
			break;
		case S_IFREG:
            // pr_alert("create a new file inode.\n");
			inode->i_op = &file_inode_ops;
			inode->i_fop = &file_ops;
			break;
		case S_IFDIR:
            // pr_alert("create a new dir inode.\n");
			inode->i_op = &dir_inode_ops;
			inode->i_fop = &dir_file_ops;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		}
	}
	return inode;
}

/**
 * open时会指定路径名，需要循环解析：根据当前dentry得到当前分量的dentry
 * 当在dcache找不到指定dentry时，会创建一个新的dentry并调用此方法
 * 参数：dir为父节点，dentry为子项（初始状态：count：1，flags：LOOK_UP）
 * 需要调用d_add方法：
 *  1.去掉LOOK_UP标记
 *  2.如果inode不为空：
 *      dentry->d_inode = inode
 *      根据inode的属性配置dentry的新flags
 *      加入到dir的i_dentry链表中
 *  3.如果inode为空：则dentry自动成为MISS状态（负状态）
 *  4.将dentry加入到dcache中
 * 返回后都会执行一次dput
 * 补充：
 *  如果是从dcache中取出但不符合要求的dentry，此时count为0（因为上一次lookup之后有dput）,
 *  会先将count+1，再执行一遍dput
 */

struct dentry *pdir_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    pdentry_t *pdentry;
    struct inode *inode;

    pdentry = disk_lookup(dir->i_ino, dentry->d_name.name);
    if(!pdentry)
    {
        //加入dcache
        d_add(dentry, NULL);
        return 0;
    }
        
    inode = get_inode(dir->i_sb, dir, get_mode(pdentry->ino), pdentry->ino, get_size(pdentry->ino));
    d_add(dentry, inode);
    show_dentry = dentry;
    return 0;
}

/**
 * dput:
 *      将dentry的count减一，如果减到0时：
 *          调用d_delete方法来决定是否cached（默认为cached）：
 *              不cached：回收dentry
 *              cached：加入到sb->lru中，当dentry空间不足时才从lru中取出回收
 */

int pfile_create(struct user_namespace *mnt_userns, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    int ret;
    struct inode *inode;
    unsigned short ino = disk_create_pfile(mode);
    if(!ino)
        return -ENOMEM;

    ret = disk_create_pdentry(get_pdir(dir->i_ino), ino, dentry->d_name.name);
    if(ret)
    {
        delete_pflie(ino);
        return -ENOMEM;
    }

    inode = get_inode(dir->i_sb, dir, mode | S_IFREG, ino, 0);
    d_instantiate(dentry, inode); // ？也会加入缓存 ？
    // d_set_d_op(dentry, &dentry_ops);
    show_dentry = dentry;
    return 0;
}

int pfile_unlink(struct inode *dir, struct dentry *dentry)
{
    delete_pdentry(get_pdir(dir->i_ino), dentry->d_name.name);
    delete_pflie(dentry->d_inode->i_ino);
    drop_nlink(dentry->d_inode);
    show_dentry = dentry;
    return 0;
}

int pdir_create(struct user_namespace *mnt_userns, struct inode *dir, struct dentry *dentry, umode_t mode)
{
    int ret;
    struct inode *inode;
    unsigned short ino = disk_create_pdir(mode);
    if(!ino)
        return -ENOMEM;

    ret = disk_create_pdentry(get_pdir(dir->i_ino), ino, dentry->d_name.name);
    if(ret)
    {
        delete_pdir(ino);
        return -ENOMEM;
    }

    inode = get_inode(dir->i_sb, dir, mode | S_IFDIR, ino, 4096);
    d_instantiate(dentry, inode);
    inc_nlink(dir);
    show_dentry = dentry;
    return 0;
}

int pdir_unlink(struct inode *dir,struct dentry *dentry)
{
    if(!disk_pdir_empty(get_pdir(dentry->d_inode->i_ino)))
        return -ENOTEMPTY;

    delete_pdentry(get_pdir(dir->i_ino), dentry->d_name.name);
    delete_pdir(dentry->d_inode->i_ino);
    drop_nlink(dentry->d_inode);
    drop_nlink(dentry->d_inode); // "."
    drop_nlink(dir);
    show_dentry = dentry;
    return 0;
}

struct inode_operations dir_inode_ops = {
    .lookup = pdir_lookup,
    .create = pfile_create,
    .unlink = pfile_unlink,
    .mkdir  = pdir_create,
    .rmdir  = pdir_unlink,
};

struct inode_operations file_inode_ops = {

};