/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include "device_io.hpp"
#include <array>
namespace ext2 {

/*
 * implements the device concept
 */
template <typename Filesystem> class inode : public inode_base<typename Filesystem::device_type> {
      public:
	typedef Filesystem fs_type;

      private:
	fs_type *fs;

	uint32_t get_block_id(uint32_t block_index) const {
		if (block_index < 12) {
			// direct pointer
			return this->data.block_pointer_direct[block_index];
		} else if (block_index < 268) {
			block_index -= 12;
			uint32_t result;
			auto blockid = this->data.block_pointer_indirect[0];
			detail::read_from_device(*fs->device(), fs->to_address(blockid, block_index*sizeof(uint32_t)), result);
			return result;
		} else {
			throw "this is not ready yet";	
			return 0; // TODO: implement
		}
	}

      public:
	inode(fs_type *fs, uint64_t offset) : inode_base<typename Filesystem::device_type>(fs->device(), offset), fs(fs) {}

	inline bool is_directory() const { return this->data.type & detail::directory; }
	inline bool is_regular_file() const { return this->data.type & detail::regular_file; }
	inline bool is_symbolic_link() const { return this->data.type & detail::symbolic_link; }
	inline uint64_t size() const {
		if (is_regular_file() && fs->large_files()) {
			return (static_cast<uint64_t>(this->data.dir_acl) << 32) | (this->data.size);
		} else {
			return this->data.size;
		}
	}

	void read(uint64_t offset, char *buffer, uint64_t length) {
		auto buffer_offset = 0;
		do {
			auto block_index = offset / fs->block_size();
			auto block_offset = offset % fs->block_size();
			auto block_length = std::min(fs->block_size() - block_offset, length);
			fs->device()->read(fs->to_address(get_block_id(block_index), block_offset), &buffer[buffer_offset], block_length);
			buffer_offset += block_length;
			offset += block_length;
			length -= block_length;
		} while (length > 0);
	}
	void write(uint64_t offset, const char *buffer, uint64_t length) {} // TODO: implement!

	inline fs_type *get_fs() { return fs; }
};

typedef std::vector<detail::directory_entry> directory_entry_list;

namespace inodes {
template <typename Filesystem> struct file : inode<Filesystem> {};
template <typename OStream, typename Filesystem> OStream &operator<<(OStream &os, file<Filesystem> &f) {
	std::array<char, 255> buffer;
	auto offset = 0;
	const auto size = f.size();
	do {
		auto length = std::min<uint64_t>(size - offset, buffer.size());
		f.read(offset, buffer.data(), length);
		os << std::string(buffer.data(), length);
		offset += length;
	} while (offset < size);
	return os;
}

template <typename Filesystem> struct character_device : inode<Filesystem> {};
template <typename Filesystem> struct block_device : inode<Filesystem> {};
template <typename Filesystem> struct fifo : inode<Filesystem> {};
template <typename Filesystem> struct symbolic_link : inode<Filesystem> {};
template <typename Filesystem> struct directory : inode<Filesystem> {

	directory_entry_list read_entrys() {
		auto offset = 0;
		directory_entry_list result;
		result.reserve(8);
		do {
			detail::directory_entry entry;
			detail::read_from_device(*this, offset, entry, 8);
			if(entry.inode_id == 0) 
				break;
			offset += 8;
			entry.name.resize(entry.name_size);
			this->read(offset, const_cast<char *>(entry.name.c_str()), entry.name_size);
			offset += entry.size - 8;
			result.push_back(std::move(entry));
		} while (offset < this->size());
		return result;
	}

	void write_entrys(const directory_entry_list &entrys) {
		// TODO
	}
};
} /* namespace inodes */

template <typename Filesystem> inodes::directory<Filesystem> *to_directory(inode<Filesystem> *from) {
	if (from->is_directory())
		return static_cast<inodes::directory<Filesystem> *>(from);
	else
		return nullptr;
}
template <typename Filesystem> inodes::file<Filesystem> *to_file(inode<Filesystem> *from) {
	if (from->is_regular_file())
		return static_cast<inodes::file<Filesystem> *>(from);
	else
		return nullptr;
}

template <typename Filesystem> inodes::file<Filesystem> *to_symbolic_link(inode<Filesystem> *from) {
	if (from->is_symbolic_link())
		return static_cast<inodes::file<Filesystem> *>(from);
	else
		return nullptr;
}

typedef std::vector<std::pair<uint64_t, std::string*>> path;

template<typename OStream>
OStream& operator<<(OStream& os, const path& p) {
	for(const auto& item : p) {
		os << '/' << *item.second;
	}
	return os;
}

template <typename Device> class filesystem {
      public:
	typedef typename superblock<Device>::device_type device_type;
	typedef inode<filesystem<Device> > inode_type;
	typedef inodes::directory<filesystem<Device> > directory_type;
	typedef inodes::file<filesystem<Device> > file_type;
	typedef group_descriptor_table<Device> gd_table_type;

      private:
	superblock<Device> super_block;
	gd_table_type gd_table;
	uint32_t blocksize;

      public:
	filesystem(Device &d, uint64_t superblock_offset = 1024) : super_block(d, superblock_offset) {}

	inline device_type *device() { return super_block.device(); }

	void load() {
		super_block.load();
		blocksize = super_block.data.block_size();
		gd_table = read_group_descriptor_table(super_block);
	}

	template <typename OStream> OStream &dump(OStream &os) const {
		super_block.data.dump(os);
		for (const auto &item : gd_table) {
			item.data.dump(os);
		}
		return os;
	}

	inode_type get_inode(uint32_t inodeid) {
		auto block_group_id = (inodeid - 1) / super_block.data.inodes_per_group;
		auto index = (inodeid - 1) % super_block.data.inodes_per_group;
		auto block_id = (index * super_block.data.inode_size) / block_size();
		auto block_offset = (index * super_block.data.inode_size) - (block_id * block_size());
		block_id += gd_table[block_group_id].data.address_inode_table;
		inode_type result(this, to_address(block_id, block_offset));
		result.load();
		return result;
	}

	inode_type get_root() { return get_inode(2); }

	inline uint64_t to_address(uint32_t blockid, uint32_t block_offset) const { return (blockid * block_size()) + block_offset; }
	inline bool large_files() const { return super_block.data.large_files(); }
	inline uint32_t block_size() const { return blocksize; }
};

template <typename Device> filesystem<Device> read_filesystem(Device &d) {
	filesystem<Device> result(d);
	result.load();
	return result;
}

} /* namespace ext2 */

#endif /* __FILESYSTEM_HPP__ */
