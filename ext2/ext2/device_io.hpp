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
template <typename Device, typename T> void write_to_device(Device &device, uint64_t offset, const T &value, uint64_t length = sizeof(T)) {
	device.write(offset, reinterpret_cast<const char *>(&value), length);
}

template <typename Device, typename T> void read_from_device(Device &device, uint64_t offset, T &value, uint64_t length = sizeof(T)) {
	device.read(offset, reinterpret_cast<char *>(&value), length);
}

} /* namespace detail */

template <typename Device, typename Block> class block_data {
	std::pair<Device *, uint64_t> pos;

      public:
	using device_type = Device;
	using block_type = Block;

	Block data;

	block_data(Device *d = nullptr, uint64_t offset = 0) : pos(d, offset) {}
	block_data(const block_data<Device, Block> &) = default;
	block_data(block_data<Device, Block> &&) = default;

	void save() { detail::write_to_device(*pos.first, pos.second, data); }

	void load() { detail::read_from_device(*pos.first, pos.second, data); }

	device_type *device() const { return pos.first; }
	uint64_t offset() const { return pos.second; }
};

template <typename Device> class dynamic_block_data {

	std::pair<Device *, uint64_t> pos;
	std::vector<char> _data;

      public:
	using device_type = Device;

	char *data() { return &_data[0]; }
	const char *data() const { return &_data[0]; }

	dynamic_block_data(Device *d = nullptr, uint64_t offset = 0, uint64_t _size = 0) : pos(d, offset) { _data.resize(_size); }

	void save() { device()->write(pos.second, data(), size()); }
	void load() { device()->read(pos.second, data(), size()); }

	device_type *device() const { return pos.first; }
	uint64_t offset() const { return pos.second; }
	uint64_t size() const { return _data.size(); }
	bool empty() const { return _data.empty(); }
	void set_size(uint64_t size) { _data.resize(size); }
};
template <typename Device> class bitmap : public dynamic_block_data<Device> {
	public:

	bitmap(Device *d = nullptr, uint64_t offset = 0, uint64_t _size = 0) : dynamic_block_data<Device>(d, offset, _size) {}

	bool get(uint64_t index) const {
		auto byte = index / 8;
		auto bit = index % 8;
		uint8_t mask = 0b00000001 << bit;
		return this->data()[byte] & mask;
	}

	void set(uint64_t index, bool b) {
		auto byte = index / 8;
		auto bit = index % 8;
		uint8_t mask = 0b00000001 << bit;
		if (b) {
			this->data()[byte] = this->data()[byte] | mask;
		} else {
			mask = !mask;
			this->data()[byte] = this->data()[byte] & mask;
		}
	}

	uint64_t find(bool bit, const uint64_t start_offset) {
		auto offset = start_offset;
		do {
			if (get(offset) == bit)
				return offset;
			if (++offset == this->size())
				offset = 0;
		} while (offset != start_offset);
		return -1;
	}
};
template <typename Device> using superblock = block_data<Device, detail::superblock>;
template <typename Device> using group_descriptor = block_data<Device, detail::group_descriptor>;
template <typename Device> using group_descriptor_table = std::vector<group_descriptor<Device> >;

template <typename Device> superblock<Device> read_superblock(Device &d) {
	superblock<Device> result(&d, 1024);
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
		result.emplace_back(&d, offset);
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

template <typename Superblock> group_descriptor_table<typename Superblock::device_type> read_group_descriptor_table(const Superblock &superblock) {
	uint64_t end = superblock.offset() + sizeof(superblock.data);
	uint64_t pos = superblock.offset() + superblock.data.block_size();
	while (pos < end) {
		pos += superblock.data.block_size();
	}
	return read_vector<typename group_descriptor_table<typename Superblock::device_type>::value_type>(*superblock.device(), pos,
													  superblock.data.block_group_count());
}


} /* namespace ext2 */
#endif /* __DEVICE_IO_HPP__ */
