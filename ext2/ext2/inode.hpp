/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __INODE_HPP__
#define __INODE_HPP__

#include "device_io.hpp"
#include "error.hpp"
#include <array>

namespace ext2 {

template <typename Filesystem, typename T> class fs_data : public block_data<typename Filesystem::device_type, T> {
      public:
	typedef Filesystem fs_type;

      private:
	fs_type *_fs;

      public:
	fs_data(fs_type *fs, uint64_t offset) : block_data<typename Filesystem::device_type, T>(fs->device(), offset), _fs(fs) {}

	inline fs_type *fs() { return _fs; }
	inline const fs_type *fs() const { return _fs; }

	uint32_t get_inode_block_id() const { return this->offset() / fs()->block_size(); }
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

	void set_block_id(uint32_t block_index, uint32_t block_id) {
		if (block_index < 12) {
			// direct pointer
			this->data.block_pointer_direct[block_index] = block_id;
		} else if (block_index < 268) {
			block_index -= 12;
			auto bpi_id = this->data.block_pointer_indirect[0];
			if (bpi_id == 0) {
				/* we are going to need a new block */
				bpi_id = this->fs()->alloc_block(this->get_inode_block_id());
				this->data.block_pointer_indirect[0] = bpi_id;
				this->save();
			}
			detail::write_to_device(*(this->fs()->device()), this->fs()->to_address(bpi_id, block_index * sizeof(uint32_t)), block_id);
		} else {
			throw "this is not ready yet";
		}
	}


      public:
	inline void set_size(uint64_t new_size) {
		uint64_t old_size = this->data.size;
		if (is_regular_file() && this->fs()->large_files()) {
			this->data.size = new_size;
			new_size >>= 32;
			this->data.dir_acl = new_size;
		} else {
			if (std::numeric_limits<uint32_t>::max() < new_size) {
				throw error::file_is_full_error();
			}
			this->data.size = new_size;
		}
		/* http://www.nongnu.org/ext2-doc/ext2.html#I-BLOCKS */
		this->data.count_sector = new_size / 512;

		if(new_size < old_size) {
			// release unsued blocks
			auto block_index = new_size / this->fs()->block_size();
			block_index++;
			uint32_t blockid = get_block_id(block_index);
			while(blockid != 0) {
				this->fs()->free_block(blockid);
				set_block_id(block_index, 0);
				block_index++;
				blockid = get_block_id(block_index);
			}
		} else if(new_size > old_size) {
			//allocate new blocks
			auto block_index_start = old_size / this->fs()->block_size();
			auto block_index_end = new_size / this->fs()->block_size();
			auto block_id = get_block_id(block_index_start);
			while(block_index_start < block_index_end) {
				block_id = this->fs()->alloc_block(block_id);
				++block_index_start;
				set_block_id(block_index_start, block_id);
			}
		}

		this->save();
	}


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
	void write(uint64_t offset, const char *buffer, uint64_t length) {
		auto buffer_offset = 0;
		if (offset >= size())
			throw error::out_of_range_error();

		if(offset + length > this->size()) {
			set_size(offset + length);
		}
		do {
			auto block_index = offset / this->fs()->block_size();
			auto block_offset = offset % this->fs()->block_size();
			auto block_length = std::min(this->fs()->block_size() - block_offset, length);
			auto block_id = get_block_id(block_index);
			this->fs()->device()->write(this->fs()->to_address(block_id, block_offset), &buffer[buffer_offset], block_length);
			buffer_offset += block_length;
			offset += block_length;
			length -= block_length;
		} while (length > 0);

	} 
};
template <typename OStream, typename Inode> void read_inode_content(OStream &os, Inode &inode) {
	std::array<char, 255> buffer;
	auto offset = 0u;
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
		auto offset = 0u;
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

	void write_entrys(directory_entry_list &entrys) {
		uint64_t offset = 0;
		for (auto i = 0u; i < entrys.size(); i++) {
			detail::directory_entry &e = entrys[i];
			e.name_size = e.name.size();
			if (i + 1 == entrys.size()) {
				e.size = std::max<uint64_t>(8 + e.name_size, this->size() - offset);
			} else {
				e.size = 8 + e.name_size;
			}
			write_to_device(*this, offset, e, 8);
			this->write(offset + 8, e.name.c_str(), e.name.size());
			offset += e.size;
		}
	}
};

template <typename Filesystem> directory<Filesystem> &operator<<(directory<Filesystem> &dir, const detail::directory_entry &e) {
	auto entry_list = dir.read_entrys();
	entry_list.push_back(e);
	dir.write_entrys(entry_list);
	return dir;
}

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

} /* namespace ext2 */

#endif /* __INODE_HPP__ */
