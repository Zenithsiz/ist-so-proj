#ifndef FS_H
#define FS_H
#include <inode.h> // TfsInode
#include <dir.h>   // TfsDir
#include <stdio.h> // FILE

void init_fs(TfsInode *table, size_t len);
void destroy_fs(TfsInode *table, size_t len);
int is_dir_empty(TfsDirEntry *dirEntries);
int create(TfsInode *table, char *name, TfsInodeType nodeType);
int delete (TfsInode *table, char *name);

int lookup(TfsInode *table, char *name);
void print_tecnicofs_tree(TfsInode *table, FILE *fp);

#endif /* FS_H */
