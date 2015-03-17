/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __EXT2_HPP__
#define __EXT2_HPP__

#include <errno.h>
#include <fcntl.h>
#include <osxfuse/fuse.h>

extern "C" {
int fuse_getattr(const char *path, struct stat *stbuf);
int fuse_open(const char *path, struct fuse_file_info *fi);
int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fuse_readlink(const char* path, char* buffer, size_t buffer_size);
int fuse_release(const char * path_str, struct fuse_file_info *fi);
}

#endif /* __EXT2_HPP__ */

