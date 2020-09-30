#include <tfs.h>

// Includes
#include <stdio.h>	// stderr, fprintf
#include <string.h> // strcpy

static void split_parent_child_from_path(char* path, char** parent, char** child);

TfsFileSystem tfs_new(size_t max_inodes) {
	// Create the inode table
	TfsFileSystem fs = {.inode_table = tfs_inode_table_new(max_inodes)};

	// Create root node
	TfsInodeIdx root;
	if (tfs_inode_table_create(&fs.inode_table, TfsInodeTypeDir, &root) != TfsInodeTableErrorSuccess || root != 0) {
		fprintf(stderr, "Failed to create root");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void tfs_drop(TfsFileSystem* fs) {
	// Drop the inode table
	tfs_inode_table_drop(&fs->inode_table);
}

TfsFileSystemError tfs_create_inode(TfsFileSystem* fs, TfsInodeType type, const char* path) {
	TfsInodeIdx parent_inumber, child_inumber;
	char *		parent_name, *child_name, name_copy[TFS_DIR_MAX_FILE_NAME_LEN];
	/* use for copy */
	TfsInodeType pType;
	TfsInodeData p_data;

	strcpy(name_copy, path);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	if (tfs_find(fs, parent_name, &parent_inumber) != TfsFileSystemErrorSuccess) {
		printf("failed to create %s, invalid parent dir %s\n",
			path,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		printf("failed to create %s, parent %s is not a dir\n",
			path,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_dir_search_by_name(&p_data.dir, child_name, NULL) == TfsInodeDataDirErrorSuccess) {
		printf("failed to create %s, already exists in dir %s\n",
			child_name,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	/* create node and add entry to folder that contains new node */
	if (tfs_inode_table_create(&fs->inode_table, type, &child_inumber) != TfsInodeTableErrorSuccess) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
			child_name,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_table_dir_add_entry(&fs->inode_table, parent_inumber, child_inumber, child_name) != TfsInodeTableErrorSuccess) {
		printf("could not add entry %s in dir %s\n",
			child_name,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	return TfsFileSystemErrorSuccess;
}

TfsFileSystemError tfs_delete_inode(TfsFileSystem* fs, const char* path) {
	TfsInodeIdx parent_inumber, child_inumber;
	char *		parent_name, *child_name, name_copy[TFS_DIR_MAX_FILE_NAME_LEN];
	/* use for copy */
	TfsInodeType pType, cType;
	TfsInodeData p_data, cdata;

	strcpy(name_copy, path);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	if (tfs_find(fs, parent_name, &parent_inumber) != TfsFileSystemErrorSuccess) {
		printf("failed to create %s, invalid parent dir %s\n",
			path,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		printf("failed to delete %s, parent %s is not a dir\n",
			child_name,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_dir_search_by_name(&p_data.dir, child_name, &child_inumber) != TfsInodeDataDirErrorSuccess) {
		printf("could not delete %s, does not exist in dir %s\n",
			path,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, child_inumber, &cType, &cdata);

	if (cType == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&cdata.dir)) {
		printf("could not delete %s: is a directory and not empty\n",
			path);
		return TfsFileSystemErrorOther;
	}

	/* remove entry from folder that contained deleted node */
	if (tfs_inode_table_dir_reset_entry(&fs->inode_table, parent_inumber, child_inumber) != TfsInodeTableErrorSuccess) {
		printf("failed to delete %s from dir %s\n",
			child_name,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_table_delete(&fs->inode_table, child_inumber) != TfsInodeTableErrorSuccess) {
		printf("could not delete inode number %d from dir %s\n",
			child_inumber,
			parent_name);
		return TfsFileSystemErrorOther;
	}

	return TfsFileSystemErrorSuccess;
}

TfsFileSystemError tfs_find(TfsFileSystem* fs, const char* path, TfsInodeIdx* idx) {
	char full_path[TFS_DIR_MAX_FILE_NAME_LEN];
	char delim[] = "/";

	strcpy(full_path, path);

	/* start at root node */
	TfsInodeIdx current_inumber = 0;

	/* use for copy */
	TfsInodeType nType;
	TfsInodeData data;

	/* get root inode data */
	tfs_inode_table_get(&fs->inode_table, current_inumber, &nType, &data);

	char* cur_path = strtok(full_path, delim);

	/* search for all sub nodes */
	while (cur_path != NULL && tfs_inode_dir_search_by_name(&data.dir, cur_path, &current_inumber) == TfsInodeDataDirErrorSuccess) {
		tfs_inode_table_get(&fs->inode_table, current_inumber, &nType, &data);
		cur_path = strtok(NULL, delim);
	}

	if (idx != NULL) {
		*idx = current_inumber;
	}

	return TfsFileSystemErrorSuccess;
}

void tfs_print(TfsFileSystem* fs, FILE* out) {
	tfs_inode_table_print_tree(&fs->inode_table, out, 0, "");
}

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATTENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
static void split_parent_child_from_path(char* path, char** parent, char** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len - 1] == '/') {
		path[len - 1] = '\0';
	}

	for (int i = 0; i < len; ++i) {
		if (path[i] == '/' && path[i + 1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child	= path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent					  = path;
	*child					  = path + last_slash_location + 1;
}
