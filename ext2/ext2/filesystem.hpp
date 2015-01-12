/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include "device_io.hpp"
#include "error.hpp"
#include <array>
#include <boost/algorithm/string/split.hpp>
namespace ext2 {

template<typename Filesystem,typename T>
class fs_data : public block_data<typename Filesystem::device_type, T> {
      public:
	typedef Filesystem fs_type;

      private:
	fs_type *_fs;

	public:
	fs_data(fs_type *fs, uint64_t offset) : block_data<typename Filesystem::device_type, T>(fs->device(), offset), _fs(fs) {}


	fs_type* fs() { return _fs; }
	const fs_type* fs() const { return _fs; }
};



/*
 * implements the device concept
 */
template <typename Filesystem> class inode : public fs_data<Filesystem, detail::inode> {

      public:
	typedef Filesystem fs_type;
      private:
	uint32_t get_block_id(uint32_t block_index) const {
		if (block_index < 12) {
			// direct pointer
			return this->data.block_pointer_direct[block_index];
		} else if (block_index < 268) {
			block_index -= 12;
			uint32_t result;
			auto blockid = this->data.block_pointer_indirect[0];
			detail::read_from_device(*(this->fs()->device()), this->fs()->to_address(blockid, block_index * sizeof(uint32_t)), result);
			return result;
		} else {
			throw "this is not ready yet";
			return 0; // TODO: implement
		}
	}

      public:
	inode(fs_type *fs, uint64_t offset) : fs_data<Filesystem, detail::inode>(fs, offset) {}

	inline bool is_directory() const { return detail::has_flag(this->data.type, detail::directory); }
	inline bool is_regular_file() const { return detail::has_flag(this->data.type, detail::regular_file); }
	inline bool is_symbolic_link() const { return detail::has_flag(this->data.type, detail::symbolic_link); }
	inline uint64_t size() const {
		if (is_regular_file() && this->fs()->large_files()) {
			return (static_cast<uint64_t>(this->data.dir_acl) << 32) | (this->data.size);
		} else {
			return this->data.size;
		}
	}

	void read(uint64_t offset, char *buffer, uint64_t length) const {
		auto buffer_offset = 0;
		do {
			auto block_index = offset / this->fs()->block_size();
			auto block_offset = offset % this->fs()->block_size();
			auto block_length = std::min(this->fs()->block_size() - block_offset, length);
			this->fs()->device()->read(this->fs()->to_address(get_block_id(block_index), block_offset), &buffer[buffer_offset], block_length);
			buffer_offset += block_length;
			offset += block_length;
			length -= block_length;
		} while (length > 0);
	}
	void write(uint64_t offset, const char *buffer, uint64_t length) {} // TODO: implement!

};
template <typename OStream, typename Inode> void read_inode_content(OStream &os, Inode &inode) {
	std::array<char, 255> buffer;
	auto offset = 0;
	const auto size = inode.size();
	do {
		auto length = std::min<uint64_t>(size - offset, buffer.size());
		inode.read(offset, buffer.data(), length);
		os << std::string(buffer.data(), length);
		offset += length;
	} while (offset < size);
}

typedef std::vector<detail::directory_entry> directory_entry_list;

namespace inodes {
template <typename Filesystem> struct file : inode<Filesystem> {

	template <typename OStream> void read_full_file(OStream &os) { read_inode_content(os, *this); }
};
template <typename OStream, typename Filesystem> OStream &operator<<(OStream &os, file<Filesystem> &f) {
	f.read_full_file(os);
	return os;
}

template <typename Filesystem> struct character_device : inode<Filesystem> {};
template <typename Filesystem> struct block_device : inode<Filesystem> {};
template <typename Filesystem> struct fifo : inode<Filesystem> {};
template <typename Filesystem> struct symbolic_link : inode<Filesystem> {

	std::string get_target() {
		/* http://www.nongnu.org/ext2-doc/ext2.html#DEF-SYMBOLIC-LINKS */
		if (this->size() < 60) {
			const char *cstring = reinterpret_cast<char *>(&(this->data.block_pointer_direct[0]));
			std::string result(cstring, this->size());
			return result;
		}

		std::stringstream ss;
		read_inode_content(ss, *this);
		return ss.str();
	}
};

template <typename Filesystem> struct directory : inode<Filesystem> {

	directory_entry_list read_entrys() {
		auto offset = 0;
		directory_entry_list result;
		result.reserve(8);
		do {
			detail::directory_entry entry;
			detail::read_from_device(*this, offset, entry, 8);
			if (entry.inode_id == 0)
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

template <typename Filesystem> inodes::symbolic_link<Filesystem> *to_symbolic_link(inode<Filesystem> *from) {
	if (from->is_symbolic_link())
		return static_cast<inodes::symbolic_link<Filesystem> *>(from);
	else
		return nullptr;
}

struct path {
	// typedef boost::iterator_range<std::string::iterator> range_type;
	typedef std::vector<std::string> vec_type;
	std::string str;
	vec_type vec;

	inline bool is_relative() const { return !(!str.empty() && str[0] == '/'); }
};

inline path path_from_string(const std::string &p) {
	path result{p};
	if (result.str.size() > 3) {
		auto first = result.str.begin();
		auto last = --(result.str.end());
		if (*first == '/')
			first++;
		if (*last != '/')
			last++;
		auto range = boost::make_iterator_range(first, last);
		boost::split(result.vec, range, [](auto c) { return c == '/'; });
	}
	return result;
}

namespace allocator {

	template<typename NotFoundError, typename BitmapVec>
	uint32_t alloc(BitmapVec& bitmaps, uint32_t elements_per_group, uint32_t related_block_id = 0) { 
		uint64_t index = -1;
		uint32_t bg_index_start = related_block_id / elements_per_group;
		uint32_t bg_index = bg_index_start;
		uint32_t result;
		do {
			index = bitmaps[bg_index].find(false, related_block_id % elements_per_group); //TODO: respect reserved blocks
			if(index != -1) {
				result = (bg_index*elements_per_group) + index;
				break;
			}
			bg_index++;
			if(bg_index == bitmaps.size()) {
				bg_index = 0;
			}
		} while(bg_index != bg_index_start);
		if(index == -1) {
			throw NotFoundError();
		}
		bitmaps[bg_index].set(index, true);
		bitmaps[bg_index].save();
		return result;
	}
	template<typename BitmapVec>
	void free(uint32_t id, BitmapVec& bitmaps, uint32_t elements_per_group) { 
		uint32_t bg_index = id / elements_per_group;
		bitmaps[bg_index].set(id % elements_per_group, false);
		bitmaps[bg_index].save();
	}
	
} /* namespace allocator */

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
	uint32_t alloc_block(uint32_t related_block_id = 0) { 
		uint32_t result = allocator::alloc<error::no_free_block_error>(block_bitmaps, super_block.data.blocks_per_group, related_block_id);
		gd_table[result % super_block.data.blocks_per_group].data.free_blocks--;
		gd_table[result % super_block.data.blocks_per_group].save();
		super_block.data.free_block_count--;
		super_block.save();
		return result;
	}
		
	void free_block(uint32_t id) { 
		allocator::free(id, block_bitmaps, super_block.data.blocks_per_group);
		gd_table[id % super_block.data.blocks_per_group].data.free_blocks++;
		gd_table[id % super_block.data.blocks_per_group].save();
		super_block.data.free_block_count++;
		super_block.save();
	}

	uint32_t alloc_inode(uint32_t related_block_id = 0) { 
		uint32_t result = allocator::alloc<error::no_free_inode_error>(inode_bitmaps, super_block.data.inodes_per_group, related_block_id);
		gd_table[result % super_block.data.inodes_per_group].data.free_inodes--;
		gd_table[result % super_block.data.inodes_per_group].save();
		super_block.data.free_inode_count--;
		super_block.save();
		return result;
	}
	
	void free_inode(uint32_t id) { 
		allocator::free(id, inode_bitmaps, super_block.data.inode_per_group);
		gd_table[id % super_block.data.inodes_per_group].data.free_inodes++;
		gd_table[id % super_block.data.inodes_per_group].save();
		super_block.data.free_inodes_count++;
		super_block.save();
	}

	inline uint64_t to_address(uint32_t blockid, uint32_t block_offset) const { return (blockid * block_size()) + block_offset; }
	inline bool large_files() const { return super_block.data.large_files(); }
	inline uint32_t block_size() const { return blocksize; }

	template <typename OStream> OStream &dump(OStream &os) const {
		super_block.data.dump(os);
		for (const auto &item : gd_table) {
			item.data.dump(os);
		}
		for(int i = 0; i < gd_table.size(); i++) {
			gd_table[i].data.dump(os);
			os << "Allocated Blocks: ";
			auto* bitmap = &block_bitmaps[i];
			auto first = 0;
			bool last = false;
			for(int k = 0; k < bitmap->size(); k++) {
				auto val = bitmap->get(k);
				if(val == true && last == false) {
					first = (i*super_block.data.blocks_per_group)+k;
				} else if(val == false && last == true) {
					os << first << '-' << ((i*super_block.data.blocks_per_group) + k)-1 << ' ';
				}
				last = val;
			}
			os << "\nAllocated Inodes: ";
			bitmap = &inode_bitmaps[i];
			first = 0;
			last = false;
			for(int k = 0; k < bitmap->size(); k++) {
				auto val = bitmap->get(k);
				if(val == true && last == false) {
					first = i*super_block.data.inodes_per_group+k;
				} else if(val == false && last == true) {
					os << first << '-' << ((i*super_block.data.inodes_per_group) + k)-1 << ' ';
				}
				last = val;
			}
			os << "\n\n";


		}
		return os;
	}

      private:
	superblock<Device> super_block;
	gd_table_type gd_table;
	std::vector<bitmap<device_type>> block_bitmaps;
	std::vector<bitmap<device_type>> inode_bitmaps;
	uint32_t blocksize;

};

template <typename Device> filesystem<Device> read_filesystem(Device &d) {
	filesystem<Device> result(d);
	result.load();
	return result;
}

} /* namespace ext2 */

#endif /* __FILESYSTEM_HPP__ */
