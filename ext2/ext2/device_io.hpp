/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __DEVICE_IO_HPP__
#define __DEVICE_IO_HPP__

#include "structs.hpp"
#include <vector>
#include <array>

namespace ext2 {
namespace detail {
template <typename Device, typename T> void write_to_device(Device &device, uint64_t offset, const T &value, uint64_t length = sizeof(T)) {
	device.write(offset, reinterpret_cast<const char *>(&value), length);
}

template <typename Device, typename T> void read_from_device(Device &device, uint64_t offset, T &value, uint64_t length = sizeof(T)) {
	device.read(offset, reinterpret_cast<char *>(&value), length);
}

template <typename Device> void zeroing_device(Device &device, uint64_t offset, uint64_t length) {
	std::array<char, 256> z;
	std::memset(z.data(), 0, z.size());
	while (length > 0) {
		uint64_t len = std::min<uint64_t>(z.size(), length);
		device.write(offset, z.data(), len);
		length -= len;
		offset += len;
	}
}

template <typename Device> struct device_stream {

	device_stream(Device *d, uint32_t offset = 0) : d(d), offset(offset) {}

	void write(const char *buffer, uint64_t size) {
		d->write(offset, buffer, size);
		offset += size;
	}

	void read(char *buffer, uint64_t size) const {
		d->read(offset, buffer, size);
		offset += size;
	}

      private:
	Device *d;
	uint32_t offset;
};

template<typename Device> device_stream<Device>& operator<<(device_stream<Device>& os, const std::string& str) {
	os.write(str.c_str(), str.size());
	return os;
}

template<typename Device> device_stream<Device>& operator<<(device_stream<Device>& os, int value) {
	os << std::to_string(value);
	return os;
}

template <typename Device> device_stream<Device> get_device_stream(Device* device, uint32_t offset = 0) {
	return device_stream<Device>(device, offset);
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

	void save() { save(pos.second); }
	void save(uint64_t offset) { detail::write_to_device(*pos.first, offset, data); }

	void load() { detail::read_from_device(*pos.first, pos.second, data); }

	device_type *device() const { return pos.first; }
	uint64_t offset() const { return pos.second; }
	uint32_t size() const { return sizeof(Block); }
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
template <typename Device> class bitmap : dynamic_block_data<Device> {
	uint64_t _count;

      public:
	bitmap(Device *d = nullptr, uint64_t offset = 0, uint64_t _count = 0) : dynamic_block_data<Device>(d, offset, _count / 8), _count(_count) {}

	inline uint64_t count() const { return _count; }

	void load() { dynamic_block_data<Device>::load(); }

	void save() { dynamic_block_data<Device>::save(); }

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
			mask = ~mask;
			this->data()[byte] = this->data()[byte] & mask;
		}
	}

	uint64_t find(bool bit, const uint64_t start_offset) {
		auto offset = start_offset;
		do {
			if (get(offset) == bit)
				return offset;
			if (++offset == this->count())
				offset = 0;
		} while (offset != start_offset);
		return -1;
	}
};
template <typename Device> using superblock = block_data<Device, detail::superblock>;
template <typename Device> using group_descriptor = block_data<Device, detail::group_descriptor>;
template <typename Device> using group_descriptor_table = std::vector<group_descriptor<Device> >;

template <typename Device> superblock<Device> read_superblock(Device &d, uint64_t offset = 1024) {
	superblock<Device> result(&d, offset);
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
template <typename T> void write_vector(std::vector<T> &vec, uint32_t offset) {
	for (auto &item : vec) {
		item.save(offset);
		offset += item.size();
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
