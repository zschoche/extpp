/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include "device_io.hpp"
#include "error.hpp"
#include "inode.hpp"
#include <boost/algorithm/string/split.hpp>

namespace ext2 {

struct path {
	// typedef boost::iterator_range<std::string::iterator> range_type;
	typedef std::vector<std::string> vec_type;
	std::string str;
	vec_type vec;

	inline bool is_relative() const { return !(!str.empty() && str[0] == '/'); }
};

inline path path_from_string(const std::string &p) {
	path result{p};
	boost::split(result.vec, result.str, [](auto c) { return c == '/'; });
	result.vec.erase(std::remove(result.vec.begin(), result.vec.end(), ""), result.vec.end());
	return result;
}

namespace allocator {

constexpr uint64_t NOT_FOUND = static_cast<uint64_t>(-1);
template <typename NotFoundError, typename BitmapVec> uint32_t alloc(BitmapVec &bitmaps, uint32_t elements_per_group, uint32_t related_block_id = 0) {
	uint64_t index = NOT_FOUND;
	uint32_t bg_index_start = related_block_id / elements_per_group;
	uint32_t bg_index = bg_index_start;
	uint32_t result;
	do {
		index = bitmaps[bg_index].find(false, related_block_id % elements_per_group); // TODO: respect reserved blocks
		if (index != NOT_FOUND) {
			result = (bg_index * elements_per_group) + index;
			break;
		}
		bg_index++;
		if (bg_index == bitmaps.size()) {
			bg_index = 0;
		}
	} while (bg_index != bg_index_start);
	if (index == NOT_FOUND) {
		throw NotFoundError();
	}
	bitmaps[bg_index].set(index, true);
	bitmaps[bg_index].save();
	return result;
}
template <typename BitmapVec> void free(uint32_t id, BitmapVec &bitmaps, uint32_t elements_per_group) {
	uint32_t bg_index = id / elements_per_group;
	bitmaps[bg_index].set(id % elements_per_group, false);
	bitmaps[bg_index].save();
}

} /* namespace allocator */

/*
 * increments the hardlink counter of given inode and peform inode.save()
 */
template <typename Inode> detail::directory_entry create_directory_entry(std::string name, uint32_t inodeid, Inode &inode) {
	detail::directory_entry result;
	result.inode_id = inodeid;
	result.name_size = name.size();
	result.size = 8 + result.name_size;
	if (detail::has_flag(inode.data.type, detail::inode_types::regular_file)) {
		result.type = detail::directory_entry_type::regular_file;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::directory)) {
		result.type = detail::directory_entry_type::directory;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::symbolic_link)) {
		result.type = detail::directory_entry_type::symbolic_link;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::fifo)) {
		result.type = detail::directory_entry_type::fifo;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::character_device)) {
		result.type = detail::directory_entry_type::character_device;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::block_device)) {
		result.type = detail::directory_entry_type::block_device;
	} else if (detail::has_flag(inode.data.type, detail::inode_types::unix_socket)) {
		result.type = detail::directory_entry_type::socket;
	} else {
		result.type = detail::directory_entry_type::unknown_type;
	}
	result.name = std::move(name);
	inode.data.count_hard_link++;
	inode.save();
	return result;
}

template <typename Device> struct filesystem {
	typedef typename superblock<Device>::device_type device_type;
	typedef inode<filesystem<Device> > inode_type;
	typedef inodes::directory<filesystem<Device> > directory_type;
	typedef inodes::file<filesystem<Device> > file_type;
	typedef inodes::symbolic_link<filesystem<Device> > symbolic_link_type;
	typedef group_descriptor_table<Device> gd_table_type;

	filesystem(Device &d, uint64_t superblock_offset = 1024) : super_block(&d, superblock_offset) {}

	inline device_type *device() { return super_block.device(); }
	inline const device_type *device() const { return super_block.device(); }

	void load() {
		super_block.load();
		blocksize = super_block.data.block_size();
		gd_table = read_group_descriptor_table(super_block);
		block_bitmaps.reserve(gd_table.size());
		inode_bitmaps.reserve(gd_table.size());
		for (auto &item : gd_table) {
			bitmap<device_type> bbitmap(device(), to_address(item.data.address_block_bitmap, 0), super_block.data.blocks_per_group);
			bitmap<device_type> ibitmap(device(), to_address(item.data.address_inode_bitmap, 0), super_block.data.inodes_per_group);
			bbitmap.load();
			ibitmap.load();
			block_bitmaps.push_back(std::move(bbitmap));
			inode_bitmaps.push_back(std::move(ibitmap));
		}
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

	/* returns a block id in the block group if related_block_id */
	uint32_t alloc_block(uint32_t related_block_id = 1) {
		related_block_id--; // bit 0 is corresponding with block 1
		uint32_t result = allocator::alloc<error::no_free_block_error>(block_bitmaps, super_block.data.blocks_per_group, related_block_id);
		result++; // bit 0 is corresponding with block 1
		auto gdt_id = result / super_block.data.blocks_per_group;
		gd_table[gdt_id].data.free_blocks--;
		gd_table[gdt_id].save();
		super_block.data.free_block_count--;
		super_block.save();
		return result;
	}

	void free_block(uint32_t id) {
		auto gdt_id = id / super_block.data.blocks_per_group;
		id--; // bit 0 is corresponding with block 1
		allocator::free(id, block_bitmaps, super_block.data.blocks_per_group);
		gd_table[gdt_id].data.free_blocks++;
		gd_table[gdt_id].save();
		super_block.data.free_block_count++;
		super_block.save();
	}

	uint32_t alloc_inode(uint32_t related_inode_id = 1) {
		related_inode_id--; // bit 0 is corresponding with block 1
		uint32_t result = allocator::alloc<error::no_free_inode_error>(inode_bitmaps, super_block.data.inodes_per_group, related_inode_id);
		auto gdt_id = result / super_block.data.inodes_per_group;
		result++; // bit 0 is corresponding with block 1
		gd_table[gdt_id].data.free_inodes--;
		gd_table[gdt_id].save();
		super_block.data.free_inodes_count--;
		super_block.save();
		return result;
	}

	void free_inode(uint32_t id) {
		id--; // bit 0 is corresponding with block 1
		auto gdt_id = id / super_block.data.inodes_per_group;
		allocator::free(id, inode_bitmaps, super_block.data.inodes_per_group);
		gd_table[gdt_id].data.free_inodes++;
		gd_table[gdt_id].save();
		super_block.data.free_inodes_count++;
		super_block.save();
	}

	inline uint64_t to_address(uint32_t blockid, uint32_t block_offset) const { return (blockid * block_size()) + block_offset; }
	inline bool large_files() const { return super_block.data.large_files(); }
	inline uint32_t block_size() const { return blocksize; }
	// inline uint32_t blocks_per_group() const { return super_block.data.blocks_per_group; }

	template <typename OStream> OStream &dump(OStream &os) const {
		super_block.data.dump(os);
		for (const auto &item : gd_table) {
			item.data.dump(os);
		}
		for (auto i = 0u; i < gd_table.size(); i++) {
			gd_table[i].data.dump(os);
			os << "Allocated Blocks: ";
			auto *bitmap = &block_bitmaps[i];
			auto first = 0;
			bool last = false;
			for (auto k = 0u; k < bitmap->size(); k++) {
				auto val = bitmap->get(k);
				if (val == true && last == false) {
					first = (i * super_block.data.blocks_per_group) + k;
				} else if (val == false && last == true) {
					os << first+1 << '-' << ((i * super_block.data.blocks_per_group) + k) << ' ';
				}
				last = val;
			}
			os << "\nAllocated Inodes: ";
			bitmap = &inode_bitmaps[i];
			first = 0;
			last = false;
			for (auto k = 0u; k < bitmap->size(); k++) {
				auto val = bitmap->get(k);
				if (val == true && last == false) {
					first = i * super_block.data.inodes_per_group + k;
				} else if (val == false && last == true) {
					os << first+1 << '-' << ((i * super_block.data.inodes_per_group) + k) << ' ';
				}
				last = val;
			}
			os << "\n\n";
		}
		return os;
	}

	std::pair<uint32_t, inode_type> create_file(uint64_t permissions = detail::inode_permissions_default, uint16_t uid = 0, uint16_t gid = 0,
						    uint32_t flags = 0) {
		return create_inode(detail::inode_types::regular_file, permissions, uid, gid, flags);
	}

      private:
	superblock<Device> super_block;
	gd_table_type gd_table;
	std::vector<bitmap<device_type> > block_bitmaps;
	std::vector<bitmap<device_type> > inode_bitmaps;
	uint32_t blocksize;

	std::pair<uint32_t, inode_type> create_inode(detail::inode_types type, uint64_t permissions = detail::inode_permissions_default, uint16_t uid = 0,
						     uint16_t gid = 0, uint32_t flags = 0) {
		auto inodeid = alloc_inode();
		auto inode = get_inode(inodeid);
		inode.data.type = type | permissions;
		inode.data.uid = uid;
		inode.data.size = 0;
		inode.data.access_time_last = 0; // TODO: we do not know the time yet
		inode.data.creation_time = 0;
		inode.data.mod_time = 0;
		inode.data.del_time = 0;
		inode.data.gid = gid;
		inode.data.count_hard_link = 0;
		inode.data.count_sector = 0;
		inode.data.flags = flags;
		inode.data.os_specific_1 = detail::os::LINUX;
		for (auto &item : inode.data.block_pointer_direct) {
			item = 0;
		}
		for (auto &item : inode.data.block_pointer_indirect) {
			item = 0;
		}
		inode.data.number_generation = 0;
		inode.data.file_acl = 0;
		inode.data.dir_acl = 0;
		inode.data.fragment_address = 0;
		inode.data.os_specific_2.linux.fragment_number = 0;
		inode.data.os_specific_2.linux.fragment_size = 0;
		inode.data.os_specific_2.linux.padding = 0;
		inode.data.os_specific_2.linux.uid_high = 0;
		inode.data.os_specific_2.linux.gid_high = 0;
		inode.data.os_specific_2.linux.padding2 = 0;

		return std::make_pair(inodeid, inode);
	}
};

template <typename Device> filesystem<Device> read_filesystem(Device &d) {
	filesystem<Device> result(d);
	result.load();
	return result;
}

} /* namespace ext2 */

#endif /* __FILESYSTEM_HPP__ */
