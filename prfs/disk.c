#include "prfs.h"

static pdir_t **pdir_arr;
static pfile_t **pfile_arr;

umode_t get_mode(unsigned short ino)
{
    umode_t mode = 0;
    if(IS_PDIR(ino))
    {
        pdir_t *pdir = get_pdir(ino);
        if(pdir)
            mode = pdir->mode;
    } else {
        pfile_t *pfile = get_pfile(ino);
        if(pfile)
            mode = pfile->mode;
    }
    return mode;
}

unsigned long get_size(unsigned short ino)
{
    unsigned long size = 0;
    if(IS_PDIR(ino))
    {
        size = 4096;
    } else {
        pfile_t *pfile = get_pfile(ino);
        if(pfile)
            size = pfile->size;
    }
    return size;
}

pdir_t *get_pdir(unsigned short ino)
{
    if(ino > MAX_ENTRY_INO)
        return 0;
    return pdir_arr[ino - 1];
}

pfile_t *get_pfile(unsigned short ino)
{
    if(ino < MAX_ENTRY_INO || ino > 1024)
        return 0;
    return pfile_arr[ino - MAX_ENTRY_INO - 1];
}

char *get_page_from_pfile(pfile_t *pfile, unsigned int index)
{
    if(!pfile || index >= MAX_FILE_PAGE)
        return 0;

    if(!pfile->pages[index])
        pfile->pages[index] = (char *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);

    return pfile->pages[index];    
}

void pfile_pages_truncate(pfile_t *pfile)
{
    int j;

    if(!pfile)
        return;

    for(j = 0; j < MAX_FILE_PAGE; j++)
        if(pfile->pages[j])
        {
            free_pages((unsigned long)pfile->pages[j], 0);
            pfile->pages[j] = 0;
        }
            
    pfile->size = 0;
}

void delete_pflie(unsigned short ino)
{
    pfile_t *pfile = get_pfile(ino);
    if(!pfile)
        return;

    pfile_pages_truncate(pfile);
    kfree(pfile);
    pfile_arr[ino - MAX_ENTRY_INO - 1] = 0;
}

void delete_pdentry(pdir_t *pdir, const char *name)
{
    int i;
    if(!pdir)
        return;
    
    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
        if(pdir->flags[i] && !strcmp(name, pdir->pdentry[i].name))
        {
            memset(&pdir->pdentry[i], 0, sizeof(pdentry_t));
            pdir->flags[i] = 0;
            break;
        }
}

unsigned short disk_create_pfile(umode_t mode)
{
    int i;
    for(i = 0; i < MAX_ENTRY_INO; i++)
        if(!pfile_arr[i])
            break;
    if(i == MAX_ENTRY_INO)
        return 0;

    pfile_arr[i] = kmalloc(sizeof(pfile_t), GFP_KERNEL | __GFP_ZERO);
    if(!pfile_arr[i])
        return 0;

    pfile_arr[i]->mode = mode | S_IFREG;
    return i + MAX_ENTRY_INO + 1;
}

int disk_create_pdentry(pdir_t *pdir, unsigned short ino, const char *name)
{
    int i;

    if(!pdir)
        return -EFAULT;

    if(!pdir->pdentry)
        pdir->pdentry = (pdentry_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!pdir->pdentry)
        return -ENOMEM;

    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
        if(!pdir->flags[i])
            break;
    if(i == MAX_DIR_ENTRY_SIZE)
        return -ENOMEM;

    pdir->pdentry[i].ino = ino;
    snprintf(pdir->pdentry[i].name, MAX_ENTRY_NAME_SIZE, name);
    pdir->flags[i] = 1;
    return 0;
}

pdentry_t *disk_lookup(unsigned short ino, const char *name)
{
    int i;
    pdir_t *pdir;

    if(ino > MAX_ENTRY_INO || !(pdir = pdir_arr[ino - 1]))
        return 0;

    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
        if(pdir->flags[i] && !strcmp(name, pdir->pdentry[i].name))
            return &pdir->pdentry[i];

    return 0;
}

void disk_destroy(void)
{
    int i;

    if(!pfile_arr)
        goto pdir_arr_destroy;

    for(i = 0; i < MAX_ENTRY_INO; i++)
    {
        if(!pfile_arr[i])
            continue;
        pfile_pages_truncate(pfile_arr[i]);
        kfree(pfile_arr[i]);
    }

    free_pages((unsigned long)pfile_arr, 0);
    pfile_arr = 0;

    pdir_arr_destroy:

    if(!pdir_arr)
        goto out;

    for(i = 0; i < MAX_ENTRY_INO; i++)
    {
        if(!pdir_arr[i])
            continue;

        if(pdir_arr[i]->pdentry)
            free_pages((unsigned long)pdir_arr[i]->pdentry, 0);

        kfree(pdir_arr[i]);
    }

    free_pages((unsigned long)pdir_arr, 0);
    pdir_arr = 0;

    // pr_alert("disk destroy\n");

    out:
    return;
}

int disk_init(umode_t root_mode, const char *root_filename, const char *root_content)
{
    pdir_t *root_dir;
    pfile_t *root_file;

    pdir_arr = (pdir_t **)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!pdir_arr)
        goto fail;

    pfile_arr = (pfile_t **)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!pfile_arr)
        goto fail;

    //创建一个根目录（目录号为1）以及根文件（文件号为513）

    pdir_arr[0] = kmalloc(sizeof(pdir_t), GFP_KERNEL | __GFP_ZERO);
    if(!pdir_arr[0])
        goto fail;

    root_dir = pdir_arr[0];

    root_dir->pdentry = (pdentry_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!root_dir->pdentry)
        goto fail;

    root_dir->flags[0] = 1;
    root_dir->mode = root_mode | S_IFDIR;
    root_dir->pdentry[0].ino = 513;
    snprintf(root_dir->pdentry[0].name, MAX_ENTRY_NAME_SIZE, root_filename);

    pfile_arr[0] = kmalloc(sizeof(pfile_t), GFP_KERNEL | __GFP_ZERO);
    if(!pfile_arr[0])
        goto fail;

    root_file = pfile_arr[0];

    root_file->pages[0] = (char *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(! root_file->pages[0])
        goto fail;

    root_file->size = strlen(root_content);
    if(root_file->size > 4096)
        root_file->size = 4096;
    root_file->mode = root_mode | S_IFREG;
    snprintf(root_file->pages[0], 4096, root_content);

    // pr_alert("root file. ino: %d name: %s\n",
    //     pdir_arr[0]->pdentry[0].ino, pdir_arr[0]->pdentry[0].name);
    return 0;

    fail:
        disk_destroy();
        return -ENOMEM;
}
