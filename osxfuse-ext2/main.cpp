#include <osxfuse/fuse.h>
#include "ext2.hpp"
#include "common.hpp"

struct fuse_operations readonly_operations = {
	.open       	= fuse_open,
	.read       	= fuse_read,
	.getattr    	= fuse_getattr,
	.release    	= fuse_release,
	.readdir       	= fuse_readdir,
	.readlink	= fuse_readlink
};

filesystem_type* fs;
filehandle_table_type fd_table;
int fd_next = 5;

/*
 * usage: e2mount mountpoint [options] ext2-image
 */
int main(int argc, char **argv) { 

	std::string filename = argv[--argc];
	device_type d(filename);
	fs = new filesystem_type(d);
	fs->load();
	return fuse_main(argc, argv, &readonly_operations, NULL); 
}


