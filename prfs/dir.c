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

struct dentry *dir_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    pdentry_t *pdentry;
    struct inode *inode;
    umode_t mode = 0;
    unsigned long size = 0;

    // pr_alert("lookup inode_i_ino: %ld dentry_name: %s flags: %x\n",
    //     dir->i_ino, dentry->d_name.name, flags);

    pdentry = disk_lookup(dir->i_ino, dentry->d_name.name);
    if(!pdentry)
    {
        // pr_alert("file %s don`t exit.\n", dentry->d_name.name);
        return 0;
    }
        

    if(IS_PDIR(pdentry->ino))
    {
        pdir_t *pdir = get_pdir(pdentry->ino);
        if(pdir)
            mode = pdir->mode;
        mode |= S_IFDIR;
        size = 4096;
    } else {
        pfile_t *pfile = get_pfile(pdentry->ino);
        if(pfile)
            mode = pfile->mode;
        mode |= S_IFREG;
        size = pfile->size;
    }

    inode = get_inode(dir->i_sb, dir, mode, pdentry->ino, size);
    d_add(dentry, inode);
    pr_alert("file %s lookup successful.\n", dentry->d_name.name);
    return 0;
}


struct inode_operations dir_inode_ops = {
    .lookup = dir_lookup
};

struct file_operations dir_file_ops = {

};