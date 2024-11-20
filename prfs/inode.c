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

struct dentry *pdir_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    pdentry_t *pdentry;
    struct inode *inode;

    // pr_alert("lookup inode_i_ino: %ld dentry_name: %s flags: %x\n",
    //     dir->i_ino, dentry->d_name.name, flags);

    pdentry = disk_lookup(dir->i_ino, dentry->d_name.name);
    if(!pdentry)
        return 0;
        
    inode = get_inode(dir->i_sb, dir, get_mode(pdentry->ino), pdentry->ino, get_size(pdentry->ino));
    d_add(dentry, inode);
    return 0;
}

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
    d_add(dentry, inode);
    return 0;
}

int pfile_unlink(struct inode *dir, struct dentry *dentry)
{
    delete_pdentry(get_pdir(dir->i_ino), dentry->d_name.name);
    delete_pflie(dentry->d_inode->i_ino);
    drop_nlink(dentry->d_inode);
    dput(dentry);
    return 0;
}

struct inode_operations dir_inode_ops = {
    .lookup = pdir_lookup,
    .create = pfile_create,
    .unlink = pfile_unlink
};

struct inode_operations file_inode_ops = {

};