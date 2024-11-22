#include "prfs.h"

/**
 * open前dentry计数加一
 */
int pfile_open(struct inode *inode, struct file *filp)
{
    pfile_t *pfile = get_pfile(inode->i_ino);
    if(!pfile)
        return -EFAULT;

    if((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        pfile_pages_truncate(pfile);
        inode->i_size = 0;
    }

    filp->private_data = pfile;
    return 0;
}

/*
* release后dentry计数会减一
*/
int pfile_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t pfile_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    pfile_t *pfile = filp->private_data;
    int index, rest;
    char *page;

    if(*f_pos >= pfile->size)
        return 0;

    index = *f_pos / 4096;
    rest = *f_pos % 4096;

    if(*f_pos + count > pfile->size)
        count = pfile->size - *f_pos;

    if(count + rest > 4096)
        count = 4096 - rest;

    page = pfile->pages[index];

    if(!page)
    {
        /*
            file black hole
        */
       if(clear_user(buf, count) != count)
            return -EFAULT;
    } else {
        if(copy_to_user(buf, page + rest, count))
            return -EFAULT;
    }

    *f_pos += count;
    return count;
}

ssize_t pfile_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    pfile_t *pfile = filp->private_data;
    int index, rest;
    char *page;

    if(*f_pos >= MAX_FILE_SIZE)
        return -ENOMEM;

    index = *f_pos / 4096;
    rest = *f_pos % 4096;

    if(count + rest > 4096)
        count = 4096 - rest;

    page = get_page_from_pfile(pfile, index);
    if(!page)
        return -ENOMEM;

    if(copy_from_user(page + rest, buf, count))
        return -EFAULT;

    *f_pos += count;
    if(pfile->size < *f_pos)
        filp->f_inode->i_size = pfile->size = *f_pos;

    return count;
}

struct file_operations file_ops = {
    .open = pfile_open,
    .release = pfile_release,
    .read = pfile_read,
    .write = pfile_write
};

int pdir_open(struct inode *inode, struct file *filp)
{
    pdir_t *pdir = get_pdir(inode->i_ino);
    if(!pdir)
        return -EFAULT;
    filp->private_data = pdir;
    return 0;
}

int pdir_release(struct inode *inode, struct file *filp)
{
    return 0;
}

int pdir_readdir(struct file *filp, struct dir_context *ctx)
{
    pdir_t *pdir = filp->private_data;
    pdentry_t *pdentry = pdir->pdentry;
    int i, avail, pos, ret;

    if(ctx->pos == 0)
    {
        ret = ctx->actor(ctx, ".", 1, 0, filp->f_path.dentry->d_inode->i_ino, DT_DIR);
        if(ret)
            return 0;
        ctx->pos = 1;
    }

    if(ctx->pos == 1)
    {
        ret = ctx->actor(ctx, "..", 2, 1, parent_ino(filp->f_path.dentry), DT_DIR);
        if(ret)
            return 0;
        ctx->pos = 2;
    }

    avail = -1;
    pos = ctx->pos - 2;
    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
    {
        if(!pdir->flags[i] || ++avail < pos)
            continue;

        ret = ctx->actor(ctx, pdentry[i].name, strlen(pdentry[i].name), ctx->pos, 
            pdentry[i].ino, S_DT(get_mode(pdentry[i].ino)));
        
        if(ret)
            break;
        ctx->pos++;
    }
    return 0;
}

struct file_operations dir_file_ops = {
    .open = pdir_open,
    .release = pdir_release,
    .iterate = pdir_readdir
};
