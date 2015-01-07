/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __DEVICE_IO_HPP__
#define __DEVICE_IO_HPP__

#include "structs.hpp"


namespace ext2 {
namespace detail {
template <typename Device, typename T> void write_to_disk(Device &device, uint64_t offset, const T &value) {
	device.write(offset, reinterpret_cast<const char *>(&value), sizeof(value));
}

template <typename Device, typename T> void read_from_disk(Device &device, uint64_t offset, T &value) {
	device.read(offset, reinterpret_cast<char *>(&value), sizeof(value));
}

} /* namespace detail */


template <typename Device, typename Block> class block_data {
	std::pair<Device *, uint64_t> pos;

      public:
	using device_type = Device;
	using block_type = Block;

	Block data;

	block_data(Device *d, uint64_t offset) : pos(d, offset) {}
	block_data(Device &d, uint64_t offset) : pos(&d, offset) {}
	block_data(Device *d) : pos(&d, -1) {}
	block_data() : pos(nullptr, -1) {}
	block_data(block_data<Device, Block> &) = default;
	block_data(block_data<Device, Block> &&) = default;

	void save() { detail::write_to_disk(*pos.first, pos.second, data); }

	void load() { detail::read_from_disk(*pos.first, pos.second, data); }

	device_type *device() const { return pos.first; }
	uint64_t offset() const { return pos.second; }
};

template <typename Device> using superblock = block_data<Device, detail::superblock>;
template <typename Device> using group_descriptor = block_data<Device, detail::group_descriptor>;
template <typename Device> using group_descriptor_table = std::vector<group_descriptor<Device> >;
template <typename Device> using inode_base = block_data<Device, detail::inode>;

template <typename Device> superblock<Device> read_superblock(Device &d) {
	superblock<Device> result(d, 1024);
	result.load();
	return result;
}

/*
 * T must be block_data<> type
 */
template <typename T> std::vector<T> read_vector(typename T::device_type &d, uint64_t offset, size_t count) {
	std::vector<T> result;
	result.reserve(count); // we want only one memory allocation

	for (size_t i = 0; i < count; i++) {
		result.emplace_back(d, offset);
		result.back().load();
		offset += sizeof(typename T::block_type);
	}

	return result;
}

template <typename T> void write_vector(std::vector<T> &vec) {
	for (auto &item : vec) {
		item.save();
	}
}

/*
 * Superblock must be superblock<> type
 */
template <typename Superblock> group_descriptor_table<typename Superblock::device_type> read_group_descriptor_table(const Superblock &superblock) {
	uint64_t end = superblock.offset() + sizeof(superblock.data);
	uint64_t pos = superblock.offset() + superblock.data.block_size();
	while(pos < end) {
		pos += superblock.data.block_size();
	}
	return read_vector<typename group_descriptor_table<typename Superblock::device_type>::value_type>(
	    *superblock.device(), pos, superblock.data.block_group_count());
}


} /* namespace ext2 */
#endif /* __DEVICE_IO_HPP__ */

