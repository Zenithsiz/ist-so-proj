#include <inode.h>

void tfs_inode_table_init(TfsInode *table, size_t len)
{
	// Empty inode, used to assign to each element in the table
	const TfsInode empty_inode = {.type = TfsInodeTypeNone};

	// Assign each table to an empty inode
	for (size_t n = 0; n < len; n++)
	{
		table[n] = empty_inode;
	}
}

void tfs_inode_table_drop(const TfsInode *table, size_t len)
{
	for (size_t n = 0; n < len; n++)
	{
		switch (table[n].type)
		{
		// Deallocate a file's contents
		case TfsInodeTypeFile:
			free(table[n].data.file.contents);
			break;

		// Deallocate a directory's children
		case TfsInodeTypeDir:
			free(table[n].data.dir.children);
			break;

		case TfsInodeTypeNone:
		default:
			break;
		}
	}
}
