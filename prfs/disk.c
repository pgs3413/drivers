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

int disk_pdir_empty(pdir_t *pdir)
{
    int i;

    if(!pdir || !pdir->pdentry)
        return 1;
    
    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
        if(pdir->flags[i])
            return 0;
    
    return 1;
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

void delete_pfile(unsigned short ino)
{
    pfile_t *pfile = get_pfile(ino);
    if(!pfile)
        return;

    if(--pfile->nrlink)
        return;
    pfile_pages_truncate(pfile);
    kfree(pfile);
    pfile_arr[ino - MAX_ENTRY_INO - 1] = 0;
}

void reino_pdentry(pdir_t *pdir, const char *name, unsigned short ino)
{
    int i;
    if(!pdir)
        return;
    
    for(i = 0; i < MAX_DIR_ENTRY_SIZE; i++)
        if(pdir->flags[i] && !strcmp(name, pdir->pdentry[i].name))
        {
            pdir->pdentry[i].ino = ino;
            break;
        }
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

void delete_pdir(unsigned short ino)
{
    pdir_t *pdir = get_pdir(ino);
    if(!pdir)
        return;

    if(pdir->pdentry)
        free_pages((unsigned long)pdir->pdentry, 0);
    kfree(pdir);
    pdir_arr[ino - 1] = 0;
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
    pfile_arr[i]->nrlink = 1;
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

unsigned short disk_create_pdir(umode_t mode)
{
    int i;
    for(i = 0; i < MAX_ENTRY_INO; i++)
        if(!pdir_arr[i])
            break;
    if(i == MAX_ENTRY_INO)
        return 0;

    pdir_arr[i] = kmalloc(sizeof(pdir_t), GFP_KERNEL | __GFP_ZERO);
    if(!pdir_arr[i])
        return 0;

    pdir_arr[i]->mode = mode | S_IFDIR;
    return i + 1;
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

    out:
    return;
}

int disk_init(umode_t root_mode, const char *root_filename1, const char *root_filename2, const char *root_content)
{
    pdir_t *root_dir;
    pfile_t *root_file;
    unsigned short dir_ino, file_ino;
    char *page;

    pdir_arr = (pdir_t **)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!pdir_arr)
        goto fail;

    pfile_arr = (pfile_t **)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);
    if(!pfile_arr)
        goto fail;

    //创建一个根目录以及根文件（但是有两个硬链接）
    file_ino = disk_create_pfile(root_mode);
    if(!file_ino)
        goto fail;

    root_file = get_pfile(file_ino);
    page = get_page_from_pfile(root_file, 0);
    if(!page)
        goto fail;

    root_file->size = strlen(root_content);
    if(root_file->size > 4096)
        root_file->size = 4096;
    snprintf(page, 4096, root_content);


    dir_ino = disk_create_pdir(root_mode);
    if(!dir_ino)
        goto fail;

    root_dir = get_pdir(dir_ino);
    //创建两个入口
    root_file->nrlink = 2;
    disk_create_pdentry(root_dir, file_ino, root_filename1);
    disk_create_pdentry(root_dir, file_ino, root_filename2);      

    return 0;

    fail:
        disk_destroy();
        return -ENOMEM;
}
