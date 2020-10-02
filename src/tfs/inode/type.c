#include "type.h"

const char* tfs_inode_type_str(TfsInodeType type) {
	switch (type) {
		case TfsInodeTypeFile:
			return "File";

		case TfsInodeTypeDir:
			return "Directory";

		case TfsInodeTypeNone:
		default: return "None";
	}
}
