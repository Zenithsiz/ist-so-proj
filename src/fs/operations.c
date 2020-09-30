//#include "operations.h"
#include <fs/operations.h>
#include <inode/inode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS_ROOT 0

#define FREE_INODE		 -1
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES	 20

#define SUCCESS 0
#define FAIL	-1

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

/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs(TfsInodeTable table) {
	tfs_inode_table_init(table);

	/* create root inode */
	TfsInodeIdx root;
	tfs_inode_table_create(table, TfsInodeTypeDir, &root);

	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs(TfsInodeTable table) {
	tfs_inode_table_drop(table);
}

/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(TfsDirEntry* dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inode_idx != (TfsInodeIdx)FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}

/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
static int lookup_sub_node(char* name, TfsDirEntry* entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (entries[i].inode_idx != (TfsInodeIdx)FREE_INODE && strcmp(entries[i].name, name) == 0) {
			return entries[i].inode_idx;
		}
	}
	return FAIL;
}

/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(TfsInodeTable table, char* name, TfsInodeType nodeType) {
	TfsInodeIdx parent_inumber, child_inumber;
	char *		parent_name, *child_name, name_copy[TFS_DIR_MAX_FILE_NAME_LEN];
	/* use for copy */
	TfsInodeType pType;
	TfsInodeData p_data;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(table, parent_name);

	if (parent_inumber == (TfsInodeIdx)FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
			name,
			parent_name);
		return FAIL;
	}

	tfs_inode_table_get(table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		printf("failed to create %s, parent %s is not a dir\n",
			name,
			parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, p_data.dir.entries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
			child_name,
			parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	tfs_inode_table_create(table, nodeType, &child_inumber);
	if (child_inumber == (TfsInodeIdx)FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
			child_name,
			parent_name);
		return FAIL;
	}

	if (tfs_inode_table_dir_add_entry(table, parent_inumber, child_inumber, child_name) != TfsInodeTableErrorSuccess) {
		printf("could not add entry %s in dir %s\n",
			child_name,
			parent_name);
		return FAIL;
	}

	return SUCCESS;
}

/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete (TfsInodeTable table, char* name) {
	int	  parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[TFS_DIR_MAX_FILE_NAME_LEN];
	/* use for copy */
	TfsInodeType pType, cType;
	TfsInodeData p_data, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(table, parent_name);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
			child_name,
			parent_name);
		return FAIL;
	}

	tfs_inode_table_get(table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		printf("failed to delete %s, parent %s is not a dir\n",
			child_name,
			parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, p_data.dir.entries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
			name,
			parent_name);
		return FAIL;
	}

	tfs_inode_table_get(table, child_inumber, &cType, &cdata);

	if (cType == TfsInodeTypeDir && is_dir_empty(cdata.dir.entries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
			name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (tfs_inode_table_dir_reset_entry(table, parent_inumber, child_inumber) != TfsInodeTableErrorSuccess) {
		printf("failed to delete %s from dir %s\n",
			child_name,
			parent_name);
		return FAIL;
	}

	if (tfs_inode_table_delete(table, child_inumber) != TfsInodeTableErrorSuccess) {
		printf("could not delete inode number %d from dir %s\n",
			child_inumber,
			parent_name);
		return FAIL;
	}

	return SUCCESS;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(TfsInodeTable table, char* name) {
	char full_path[TFS_DIR_MAX_FILE_NAME_LEN];
	char delim[] = "/";

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	TfsInodeType nType;
	TfsInodeData data;

	/* get root inode data */
	tfs_inode_table_get(table, current_inumber, &nType, &data);

	char* path = strtok(full_path, delim);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dir.entries)) != FAIL) {
		tfs_inode_table_get(table, current_inumber, &nType, &data);
		path = strtok(NULL, delim);
	}

	return current_inumber;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(TfsInodeTable table, FILE* fp) {
	tfs_inode_table_print_tree(table, fp, FS_ROOT, "");
}
