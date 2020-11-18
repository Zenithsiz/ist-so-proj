#include "fs.h"

void tfs_fs_find_error_print(const TfsFsFindError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsFindErrorParentsNotDir: {
			const TfsPath* path = &self->data.parents_not_dir.path;
			fprintf(out, "Entry '%.*s' is not a directory\n", (int)path->len, path->chars);
			break;
		}

		case TfsFsFindErrorNameNotFound: {
			const TfsPath* path = &self->data.name_not_found.path;
			fprintf(out, "Entry '%.*s' does not exist\n", (int)path->len, path->chars);
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_fs_create_error_print(const TfsFsCreateError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsCreateErrorInexistentParentDir: {
			fprintf(out, "Unable to find parent directory\n");
			tfs_fs_find_error_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsCreateErrorParentNotDir: {
			fprintf(out, "Parent directory was not a directory\n");
			break;
		}

		case TfsFsCreateErrorAddEntry: {
			fprintf(out, "Unable to add directory entry\n");
			tfs_inode_dir_add_entry_error_print(&self->data.add_entry.err, out);
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_fs_remove_error_print(const TfsFsRemoveError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsRemoveErrorInexistentParentDir: {
			fprintf(out, "Unable to find parent directory\n");
			tfs_fs_find_error_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsRemoveErrorParentNotDir: {
			fprintf(out, "Parent directory was not a directory\n");
			break;
		}

		case TfsFsRemoveErrorNameNotFound: {
			fprintf(out, "Cannot find entry in parent directory\n");
			break;
		}

		case TfsFsRemoveErrorRemoveNonEmptyDir: {
			fprintf(out, "Directory was not empty\n");
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_fs_move_error_print(const TfsFsMoveError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsMoveErrorInexistentCommonAncestor: {
			fprintf(out, "The common ancestor of both paths was not found\n");
			break;
		}
		case TfsFsMoveErrorCommonAncestorNotDir: {
			fprintf(out, "The common ancestor of both paths was not a directory.");
			break;
		}
		case TfsFsMoveErrorOriginDestinationParent: {
			fprintf(out, "The origin path was the destination path's parent\n");
			break;
		}
		case TfsFsMoveErrorDestinationOriginParent: {
			fprintf(out, "The destination path was the origin's path parent\n");
			break;
		}
		case TfsFsMoveErrorInexistentOriginParentDir: {
			fprintf(out, "The origin path's parent did not exist\n");
			break;
		}
		case TfsFsMoveErrorInexistentDestinationParentDir: {
			fprintf(out, "The destination path's parent did not exist\n");
			break;
		}
		case TfsFsMoveErrorOriginParentNotDir: {
			fprintf(out, "The origin path's parent was not a directory\n");
			break;
		}
		case TfsFsMoveErrorDestinationParentNotDir: {
			fprintf(out, "The destination path's parent was not a directory\n");
			break;
		}
		case TfsFsMoveErrorOriginNotFound: {
			fprintf(out, "The origin path was not found\n");
			break;
		}
		case TfsFsMoveErrorAddEntry: {
			fprintf(out, "Unable to add an entry to the destination path's parent\n");
			tfs_inode_dir_add_entry_error_print(&self->data.add_entry.err, out);
			break;
		}
		case TfsFsMoveErrorRenameEntry: {
			fprintf(out, "Unable to rename entry in common parent\n");
			tfs_inode_dir_rename_error_print(&self->data.rename_entry.err, out);
			break;
		}
		default: {
			break;
		}
	}
}
