/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include "device_io.hpp"

namespace ext2 {

template <typename Filesystem> class inode : public inode_base<typename Filesystem::device_type> {
      public:
	typedef Filesystem fs_type;

      private:
	fs_type *fs;

	uint32_t get_block_id(uint32_t block_index) const {
		if(block_index < 12) {
			//direct pointer
			return this->data.block_pointer_direct[block_index];
		} else {
			return 0; //TODO: ...
		}
	}

      public:
	inode(fs_type *fs, uint64_t offset) : inode_base<typename Filesystem::device_type>(fs->device(), offset), fs(fs) {}

	inline bool is_directory() const { return this->data.type & detail::directory; }
	inline bool is_regular_file() const { return this->data.type & detail::regular_file; }
	inline uint64_t size() const {
		if (is_regular_file() && fs->large_files()) {
			return (static_cast<uint64_t>(this->data.dir_acl) << 32) | (this->data.size);
		} else {
			return this->data.size;
		}
	}

	void read(uint64_t offset, char* buffer, uint64_t length) {
		do {
			auto block_index = offset / fs->block_size();
			auto block_offset = offset % fs->block_size();
			auto block_length = std::min(fs->block_size() - block_offset, length);
			fs->device()->read(fs->to_address(get_block_id(block_index), block_offset), buffer, block_length);
			offset += block_length;
			length -= block_length;
		} while (length > 0);
	}
};

template <typename Device> class filesystem {
      public:
	typedef typename superblock<Device>::device_type device_type;
	typedef inode<filesystem<Device> > inode_type;
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
