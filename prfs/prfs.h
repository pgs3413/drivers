#ifndef DISK_H
#define DISK_H

#include<linux/fs.h>
#include<linux/gfp.h>
#include<linux/slab.h>

#define MAX_GLOBAL_ENTRY_SIZE 512 //全局最多512个文件；全局最多512个目录
#define MAX_DIR_ENTRY_SIZE 128 //每一个目录下最多129个子项
#define MAX_ENTRY_NAME_SIZE 30 //文件名最大长度为29
#define MAX_ENTRY_INO 512 // 1 ~ 512为目录号 513 ~ 1024为文件号
#define MAX_FILE_PAGE 256 //文件内容最大页数为256页，即1MB

#define IS_PDIR(ino) (ino < MAX_ENTRY_INO)
#define IS_PFILE(ino) (ino > MAX_ENTRY_INO)

typedef struct pdentry {
    unsigned short ino;
    char name[MAX_ENTRY_NAME_SIZE];
} pdentry_t;

typedef struct pdir {
    struct pdentry *pdentry;
    char flags[MAX_DIR_ENTRY_SIZE];
    umode_t mode;
} pdir_t;

typedef struct pfile {
    char *pages[MAX_FILE_PAGE];
    unsigned long size;
    umode_t mode;
} pfile_t;

int disk_init(umode_t root_mode, const char *root_filename, const char *root_content);
void disk_destroy(void);
pdentry_t *disk_lookup(unsigned short ino, const char *name);
pdir_t *get_pdir(unsigned short ino);
pfile_t *get_pfile(unsigned short ino);

extern struct file_operations dir_file_ops;
extern struct inode_operations dir_inode_ops;
extern struct file_operations file_ops;
extern struct inode_operations file_inode_ops;

#endif