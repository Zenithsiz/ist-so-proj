#ifndef FS_H
#define FS_H
#include <inode.h> // TfsInode
#include <dir.h>   // TfsDir
#include <stdio.h> // FILE

void init_fs(TfsInodeTable table);
void destroy_fs(TfsInodeTable table);
int is_dir_empty(TfsDirEntry *dirEntries);
int create(TfsInodeTable table, char *name, TfsInodeType nodeType);
int delete (TfsInodeTable table, char *name);

int lookup(TfsInodeTable table, char *name);
void print_tecnicofs_tree(TfsInodeTable table, FILE *fp);

#endif /* FS_H */
