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

void insert_hash(struct inode *inode)
{
    if(insert_inode_locked(inode) < 0)
            pr_alert("insert_inode_locked failed.\n");
        else
            unlock_new_inode(inode);
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
    
    inode = ilookup(dir->i_sb, pdentry->ino);
    if(inode) {
        pr_alert("lookup %s in superblock.\n", dentry->d_name.name);
    } else {
        inode = get_inode(dir->i_sb, dir, get_mode(pdentry->ino), pdentry->ino, get_size(pdentry->ino));
        if(IS_PFILE(pdentry->ino))
        {
            inode->__i_nlink = get_pfile(pdentry->ino)->nrlink;
            insert_hash(inode);
        }
        pr_alert("lookup %s in new inode.\n", dentry->d_name.name);
    }
    d_add(dentry, inode);
    show_dentry = dentry;
    show_inode = inode;
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
        delete_pfile(ino);
        return -ENOMEM;
    }

    inode = get_inode(dir->i_sb, dir, mode | S_IFREG, ino, 0);
    insert_hash(inode);
    d_instantiate(dentry, inode); // ？也会加入缓存 ？
    show_dentry = dentry;
    show_inode = inode;
    return 0;
}

int pfile_unlink(struct inode *dir, struct dentry *dentry)
{
    delete_pdentry(get_pdir(dir->i_ino), dentry->d_name.name);
    delete_pfile(dentry->d_inode->i_ino);
    drop_nlink(dentry->d_inode);
    show_dentry = dentry;
    show_inode = dentry->d_inode;
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
    show_inode = inode;
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
    show_inode = dentry->d_inode;
    return 0;
}

int pdentry_rename(struct user_namespace *mnt_userns, struct inode *old_dir,
		  struct dentry *old_dentry, struct inode *new_dir,
		  struct dentry *new_dentry, unsigned int flags)
{
    struct inode *inode = old_dentry->d_inode;
    int ret;

    //1.文件/目录rename new_dentry不存在
    if(d_is_miss(new_dentry))
    {
        ret = disk_create_pdentry(get_pdir(new_dir->i_ino), inode->i_ino, new_dentry->d_name.name);
        if(ret)
            return ret;
        delete_pdentry(get_pdir(old_dir->i_ino), old_dentry->d_name.name);

        show_dentry = old_dentry;
        show_inode = inode;

        if(d_is_dir(old_dentry))
        {
            drop_nlink(old_dir);
            inc_nlink(new_dir);
        }
        return 0;
    }

    //2.文件rename new_dentry存在
    if(d_is_reg(old_dentry) && d_is_reg(new_dentry))
    {
        struct inode *target = new_dentry->d_inode;
        delete_pfile(target->i_ino);
        drop_nlink(target);
        delete_pdentry(get_pdir(old_dir->i_ino), old_dentry->d_name.name);
        reino_pdentry(get_pdir(new_dir->i_ino), new_dentry->d_name.name, inode->i_ino);
        show_dentry = new_dentry;
        show_inode = inode;
        return 0;
    }

    //3.目录rename new_dentry存在
    if(d_is_dir(old_dentry) && d_is_dir(new_dentry))
    {
        struct inode *target = new_dentry->d_inode;
        if(!disk_pdir_empty(get_pdir(target->i_ino)))
            return -ENOTEMPTY;

        delete_pdir(target->i_ino);
        drop_nlink(target);
        drop_nlink(target); // "."
        drop_nlink(old_dir);
        delete_pdentry(get_pdir(old_dir->i_ino), old_dentry->d_name.name);
        reino_pdentry(get_pdir(new_dir->i_ino), new_dentry->d_name.name, inode->i_ino);
        return 0;
    }

    return -EINVAL;
}

int pdentry_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = old_dentry->d_inode;
    int ret = disk_create_pdentry(get_pdir(dir->i_ino), inode->i_ino, dentry->d_name.name);
    if(ret)
        return -ENOMEM;
    get_pfile(inode->i_ino)->nrlink++;
    inc_nlink(inode);
    ihold(inode);
    d_instantiate(dentry, inode);
    d_set_d_op(dentry, &dentry_ops);
    show_dentry = dentry;
    show_inode = inode;
    return 0;
}

struct inode_operations dir_inode_ops = {
    .lookup = pdir_lookup,
    .create = pfile_create,
    .unlink = pfile_unlink,
    .mkdir  = pdir_create,
    .rmdir  = pdir_unlink,
    .rename = pdentry_rename,
    .link   = pdentry_link
};

struct inode_operations file_inode_ops = {

};