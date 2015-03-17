#include "ext2.hpp"
#include "common.hpp"

int fuse_getattr(const char *path_str, struct stat *stbuf) {
	const std::string path(path_str);
	uint32_t inode_id = 2;	
	if(path != "/") {
		auto root = fs->get_root();
		inode_id = ext2::find_inode(root, path);
	}

	if(inode_id == 0) {
		return -ENOENT;
	}
	auto inode = fs->get_inode(inode_id);	
	stbuf->st_ino = inode_id;
	stbuf->st_mode = inode.data.type;
	stbuf->st_nlink = inode.data.count_hard_link;
	stbuf->st_uid = inode.data.uid;
	stbuf->st_gid = inode.data.gid;
	stbuf->st_size = inode.size();
	stbuf->st_flags = inode.data.flags;
	stbuf->st_gen = inode.data.number_generation;
	stbuf->st_atime = inode.data.access_time_last;
	stbuf->st_mtime = inode.data.mod_time;
	stbuf->st_ctime = inode.data.creation_time;
	return 0;
}

int fuse_open(const char *path_str, struct fuse_file_info *fi) {
	const std::string path(path_str);
	uint32_t inode_id = 2;	
	if(path != "/") {
		auto root = fs->get_root();
		inode_id = ext2::find_inode(root, path, false);
	}

	if(inode_id == 0) {
		return -ENOENT;
	}
	fi->fh = ++fd_next;
	fd_table.insert({ fi->fh, fh_type{ fs->get_inode(inode_id) } });
	//TODO: check flags
	return 0;
}

int fuse_release(const char * path_str, struct fuse_file_info *fi) {
	fd_table.erase(fi->fh);
	return 0;
}

int fuse_readdir(const char *path_str, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	const std::string path(path_str);
	uint32_t inode_id = 2;	
	if(path != "/") {
		auto root = fs->get_root();
		inode_id = ext2::find_inode(root, path);
	}

	if(inode_id == 0) {
		return -ENOENT;
	}
	auto inode = fs->get_inode(inode_id);	
	if(auto* d = ext2::to_directory(&inode)) {
		auto entrys = d->read_entrys();
		for(const auto& e : entrys) {
			filler(buf, e.name.c_str(), NULL, 0);
		}
	} else {
		return -EINVAL;
	}
	return 0;
}

int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	auto iter = fd_table.find(fi->fh);
	if(iter == fd_table.end()) {
		return -ENOENT;
	}

	auto file_size = iter->second.inode.size();
	if (offset >= file_size) /* Trying to read past the end of file. */
		return 0;
	size = std::min<size_t>(size, file_size - offset);
	iter->second.inode.read(offset, buf, size);
	return size;
}

int fuse_readlink(const char* path_str, char* buffer, size_t buffer_size) {
	
	const std::string path(path_str);
	uint32_t inode_id = 2;	
	if(path != "/") {
		auto root = fs->get_root();
		inode_id = ext2::find_inode(root, path, false);
	}

	if(inode_id == 0) {
		return -ENOENT;
	}
	auto inode = fs->get_inode(inode_id);	
	if(auto* symlink = ext2::to_symbolic_link(&inode)) {
		if(symlink->size() > buffer_size) {
			return -ENAMETOOLONG;
		}
		std::strcpy(buffer, symlink->get_target().c_str());
		return symlink->size();
	} else {
		return -EINVAL;
	}
	return 0;
}
