#include <inode.h>

void tfs_inode_table_init(TfsInodeTable table)
{
	// Empty inode, used to assign to each element in the table
	const TfsInode empty_inode = {.type = TfsInodeTypeNone};

	// Assign each table to an empty inode
	for (size_t n = 0; n < table.len; n++)
	{
		table.inodes[n] = empty_inode;
	}
}

void tfs_inode_table_drop(TfsInodeTable table)
{
	for (size_t n = 0; n < table.len; n++)
	{
		switch (table.inodes[n].type)
		{
		// Deallocate a file's contents
		case TfsInodeTypeFile:
			free(table.inodes[n].data.file.contents);
			break;

		// Deallocate a directory's children
		case TfsInodeTypeDir:
			free(table.inodes[n].data.dir.entries);
			break;

		case TfsInodeTypeNone:
		default:
			break;
		}
	}
}
