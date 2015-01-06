/*
*
*
*/
#ifndef __BLOCK_DEVICE_HPP__
#define __BLOCK_DEVICE_HPP__

#include "common.hpp"
#include <algorithm>

namespace ext2 {

template<typename Device>
struct block_device : public Device {

	const uint64_t sector_size;

	template<typename... Args>
	block_device(uint32_t sector_size, Args &&... args) : Device(std::forward<Args>(args)...), sector_size(sector_size)  {}

	void write(uint64_t offset, const char* buffer, uint64_t size) {
		auto sector = offset/sector_size;
		auto sector_offset = offset - (sector*sector_size);
		while(size != 0) {
			auto count = std::min(size, sector_size-sector_offset);
			write_block(sector, sector_offset, buffer, count);

			size -= count;
			buffer += count;
			sector++;
			sector_offset = 0;
		} 
	}

	void read(uint64_t offset, char* buffer, uint64_t size) {
		auto sector = offset/sector_size;
		auto sector_offset = offset - (sector*sector_size);
		while(size != 0) {
			auto count = std::min(size, sector_size-sector_offset);
			read_block(sector, sector_offset, buffer, count);

			size -= count;
			buffer += count;
			sector++;
			sector_offset = 0;
		} 
	}


	private:

	void write_block(uint64_t sector, uint64_t offset, const char* buffer, uint64_t size) {
		if(offset+size > sector_size) {
			throw ext2_error(e_block_overflow);
		}
		Device::write((sector*sector_size)+offset, buffer, size);
	}
	void read_block(uint64_t sector, uint64_t offset, char* buffer, uint64_t size) {
		if(offset+size > sector_size) {
			throw ext2_error(e_block_overflow);
		}
		Device::read((sector*sector_size)+offset, buffer, size);
	}

};
	
} /* namespace ext2 */


#endif /* __BLOCK_DEVICE_HPP__ */

