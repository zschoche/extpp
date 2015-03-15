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
#include <sstream>

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

namespace inodes {
template <typename Filesystem> struct directory;
}

/*
 * implements the device concept
 */
template <typename Filesystem> class inode : public fs_data<Filesystem, detail::inode> {

	friend struct inodes::directory<Filesystem>;

      public:
	typedef Filesystem fs_type;

      private:
	uint32_t get_indirect_block(uint32_t block, uint32_t index, uint32_t ids_per_block, int count) const {
		if(count > 0) {
			//index = index / ids_per_block;
			--count;
			block = this->get_indirect_block(block, index / ids_per_block, ids_per_block, count);
			index = index % ids_per_block;
		} 

		uint32_t result;
		detail::read_from_device(*(this->fs()->device()), this->fs()->to_address(block, index * sizeof(uint32_t)), result);
		return result;	
	}

	uint32_t get_block_id(uint32_t block_index) const {
		if (block_index < 12) {
			// direct pointer
			return this->data.block_pointer_direct[block_index];
		} else {

			auto id_per_block = (this->fs()->block_size() / 4);
			auto idp1_cut = id_per_block + 12;
			auto idp2_cut = (id_per_block * id_per_block) + idp1_cut;
			auto idp3_cut = (id_per_block * id_per_block * id_per_block) + idp2_cut;

			uint32_t block = 0;
			uint32_t index = 0;
			int count = 0;
			if(block_index < idp1_cut) {
				block = this->data.block_pointer_indirect[0];
				count = 0;
				index = block_index - 12;
			} else if( block_index < idp2_cut) {
				block = this->data.block_pointer_indirect[1];
				count = 1;
				index = block_index - idp1_cut;
			} else if(block_index < idp3_cut) {
				block = this->data.block_pointer_indirect[2];
				count = 2;
				index = block_index - idp2_cut;
			} else {
				throw error::out_of_range_error();
			}
			return get_indirect_block(block, index, id_per_block, count);
		}
	}

	uint32_t get_or_create_indirect_block(uint32_t block, uint32_t index, uint32_t ids_per_block, int count) {
		if(count > 0) {
			//index = index / ids_per_block;
			--count;
			block = this->get_or_create_indirect_block(block, index / ids_per_block, ids_per_block, count);
			index = index % ids_per_block;

			uint32_t result;
			detail::read_from_device(*(this->fs()->device()), this->fs()->to_address(block, index * sizeof(uint32_t)), result);
			if(result == 0) {
				//we need a new block
				result = this->fs()->alloc_block(this->get_inode_block_id());
				detail::zeroing_device(*(this->fs()->device()), this->fs()->to_address(result, 0), this->fs()->block_size());
				detail::write_to_device(*(this->fs()->device()), this->fs()->to_address(block, index * sizeof(uint32_t)), result);
			}
			block = result;
		} 
		return block;
	}

	void set_block_id(uint32_t block_index, uint32_t new_block_id) {
		if (block_index < 12) {
			// direct pointer
			this->data.block_pointer_direct[block_index] = new_block_id;
			this->save();
		} else {

			auto id_per_block = (this->fs()->block_size() / 4);
			auto idp1_cut = id_per_block + 12;
			auto idp2_cut = (id_per_block * id_per_block) + idp1_cut;
			auto idp3_cut = (id_per_block * id_per_block * id_per_block) + idp2_cut;

			uint32_t block = 0;
			uint32_t index = 0;
			int count = 0;
			if(block_index < idp1_cut) {
				if(this->data.block_pointer_indirect[0] == 0) {
					block = this->fs()->alloc_block(this->get_inode_block_id());
					detail::zeroing_device(*(this->fs()->device()), this->fs()->to_address(block, 0), this->fs()->block_size());
					this->data.block_pointer_indirect[0] = block;
					this->save();
				} else {
					block = this->data.block_pointer_indirect[0];
				}
				count = 0;
				index = block_index - 12;

			} else if( block_index < idp2_cut) {
				if(this->data.block_pointer_indirect[1] == 0) {
					block = this->fs()->alloc_block(this->get_inode_block_id());
					detail::zeroing_device(*(this->fs()->device()), this->fs()->to_address(block, 0), this->fs()->block_size());
					this->data.block_pointer_indirect[1] = block;
					this->save();
				} else {
					block = this->data.block_pointer_indirect[1];
				}
				count = 1;
				index = block_index - idp1_cut;

			} else if(block_index < idp3_cut) {
				if(this->data.block_pointer_indirect[2] == 0) {
					block = this->fs()->alloc_block(this->get_inode_block_id());
					detail::zeroing_device(*(this->fs()->device()), this->fs()->to_address(block, 0), this->fs()->block_size());
					this->data.block_pointer_indirect[2] = block;
					this->save();
				} else {
					block = this->data.block_pointer_indirect[2];
				}
				count = 2;
				index = block_index - idp2_cut;

			} else {
				throw error::out_of_range_error();
			}

			if(count > 0) {
				//index = index / id_per_block;
				//--count;
				block = this->get_or_create_indirect_block(block, index / id_per_block, id_per_block, count);
				index = index % id_per_block;
			}
			detail::write_to_device(*(this->fs()->device()), this->fs()->to_address(block, index * sizeof(uint32_t)), new_block_id);	
		}
	}

      public:
	inline void set_size(uint64_t new_size) {
		uint64_t old_size = this->size();
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

		if (new_size < old_size) {
			// release unsued blocks
			auto block_index = new_size / this->fs()->block_size();
			block_index++;
			uint32_t blockid = get_block_id(block_index);
			while (blockid != 0) {
				this->fs()->free_block(blockid);
				set_block_id(block_index, 0);
				block_index++;
				blockid = get_block_id(block_index);
			}
		} else if (new_size > old_size) {
			// allocate new blocks
			auto block_index_start = old_size / this->fs()->block_size();
			auto block_index_end = new_size / this->fs()->block_size();
			auto block_id = get_block_id(block_index_start);
			if (block_id == 0) {
				// the file is empty
				block_id = this->fs()->alloc_block();
				set_block_id(block_index_start, block_id);

			}

			
			while (block_index_start < block_index_end) {
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
		if (offset > size())
			throw error::out_of_range_error();

		if (offset + length > this->size()) {
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

	std::string get_target() const {
		/* http://www.nongnu.org/ext2-doc/ext2.html#DEF-SYMBOLIC-LINKS */
		if (this->size() < 60) {
			const char *cstring = reinterpret_cast<const char *>(&(this->data.block_pointer_direct[0]));
			std::string result(cstring, this->size());
			return result;
		}

		std::stringstream ss;
		read_inode_content(ss, *this);
		return ss.str();
	}

	void set_target(const std::string &target) {
		if (this->size() < 60) {
			char *cstring = reinterpret_cast<char *>(&(this->data.block_pointer_direct[0]));
			if (target.size() < 60) {
				std::copy(target.begin(), target.end(), cstring);
				this->data.size = target.size();
			} else {
				for (auto i = 0u; i < 60; i++) {
					*cstring = 0;
					++cstring;
				}
				this->data.size = 0;
				this->write(0, target.c_str(), target.size());
			}
		} else {
			this->write(0, target.c_str(), target.size());
			if (target.size() < this->size()) {
				// cut this rest
				this->set_size(target.size());
			}
		}
		this->save();
	}
};


template <typename Filesystem> struct directory : inode<Filesystem> {

	directory_entry_list read_entrys() const {
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

	/*
	 * returns false, if the given name is equal to ".." or "." and if the given name is a directory and not empty.
	 */
	bool remove(const std::string &name) {
		auto entrys = read_entrys();
		return remove(name, entrys);
	}

	/*
	 * returns false, if the given name is equal to ".." or "." and if the given name is a directory and not empty.
	 */
	bool remove(const std::string &name, directory_entry_list &entrys) {
		if (name == ".." || name == ".")
			return false;

		auto iter = std::find_if(entrys.begin(), entrys.end(), [&name](auto &e) { return e.name == name; });
		if (iter != entrys.end()) {
			auto inode = this->fs()->get_inode(iter->inode_id);
			if (auto *dir = to_directory(&inode)) {
				if (dir->read_entrys().size() > 2) {
					return false;
				}
			}

			inode.data.count_hard_link--;
			inode.save();
			if (inode.data.count_hard_link == 0) {
				// free inode
				// TODO: set deletion time

				if (!inode.is_symbolic_link() || inode.size() >= 60) {
					// free block but do not reset the pointer to make recovery possible
					auto i = 0u;
					auto blockid = inode.get_block_id(i);
					while (blockid != 0) {
						this->fs()->free_block(blockid);
						blockid = inode.get_block_id(++i);
					}
				}
				this->fs()->free_inode(iter->inode_id);
			}
			entrys.erase(iter);
			write_entrys(entrys);
		}
		return true;
	}

      private:
};

template <typename Filesystem> directory<Filesystem> &operator<<(directory<Filesystem> &dir, const detail::directory_entry &e) {
	auto entry_list = dir.read_entrys();
	entry_list.push_back(e);
	dir.write_entrys(entry_list);
	return dir;
}

} /* namespace inodes */

inline auto find_entry_by_name(const directory_entry_list& entrys, const std::string& name) {
	return std::find_if(entrys.begin(), entrys.end(), [&name] (const auto& e) { return e.name == name; });
}

template <typename Filesystem> const inodes::directory<Filesystem> *to_directory(const inode<Filesystem> *from) {
	if (from->is_directory())
		return static_cast<const inodes::directory<Filesystem> *>(from);
	else
		return nullptr;
}
template <typename Filesystem> inodes::directory<Filesystem> *to_directory(inode<Filesystem> *from) {
	if (from->is_directory())
		return static_cast<inodes::directory<Filesystem> *>(from);
	else
		return nullptr;
}
template <typename Filesystem> const inodes::file<Filesystem> *to_file(const inode<Filesystem> *from) {
	if (from->is_regular_file())
		return static_cast<const inodes::file<Filesystem> *>(from);
	else
		return nullptr;
}
template <typename Filesystem> inodes::file<Filesystem> *to_file(inode<Filesystem> *from) {
	if (from->is_regular_file())
		return static_cast<inodes::file<Filesystem> *>(from);
	else
		return nullptr;
}

template <typename Filesystem> const inodes::symbolic_link<Filesystem> *to_symbolic_link(const inode<Filesystem> *from) {
	if (from->is_symbolic_link())
		return static_cast<const inodes::symbolic_link<Filesystem> *>(from);
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
