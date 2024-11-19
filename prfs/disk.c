#include "prfs.h"

static pdir_t **pdir_arr;
static pfile_t **pfile_arr;

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
    int i, j;

    if(!pfile_arr)
        goto pdir_arr_destroy;

    for(i = 0; i < MAX_ENTRY_INO; i++)
    {
        if(!pfile_arr[i])
            continue;

         for(j = 0; j < MAX_FILE_PAGE; j++)
            if(pfile_arr[i]->pages[j])
                free_pages((unsigned long)pfile_arr[i]->pages[j], 0);

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
    root_dir->mode = root_mode;
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
    root_file->mode = root_mode;
    snprintf(root_file->pages[0], 4096, root_content);

    // pr_alert("root file. ino: %d name: %s\n",
    //     pdir_arr[0]->pdentry[0].ino, pdir_arr[0]->pdentry[0].name);
    return 0;

    fail:
        disk_destroy();
        return -ENOMEM;
}
