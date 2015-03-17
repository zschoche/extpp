/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <unordered_map>
#include <string>
#include "../ext2/filesystem.hpp"
#include "../ext2/visitors.hpp"
#include "../test/host_node.hpp"

struct raw_device {
	raw_device(std::string filename) : filename(std::move(filename)) {
		open();	
	}
	~raw_device() {
		if(file != nullptr)
			close();
	}
	
	bool open() {
		file = fopen(filename.c_str(), "rb");
		return file != nullptr;
	}

	void close() {
		fclose(file);
		file = nullptr;
	}


	void write(uint64_t offset, const char* buffer, uint64_t size) {
		fseek(file, offset, SEEK_SET);	
		fwrite(buffer, sizeof(char), size, file);
	}

	void read(uint64_t offset, char* buffer, uint64_t size) const {
		fseek(file, offset, SEEK_SET);	
		auto r = fread(buffer, sizeof(char), size, file);
		if (r != size && feof(file)) {
			file = fopen(filename.c_str(), "wb");
		}
	}

	private:
	std::string filename;
	mutable FILE* file = nullptr;

};

template<typename Inode>
struct filehandle {
	Inode inode;

};

typedef raw_device 	device_type;
typedef ext2::filesystem<device_type> filesystem_type;
typedef filehandle<filesystem_type::inode_type> fh_type;
typedef std::unordered_map<int, fh_type> filehandle_table_type;
extern filesystem_type* fs;
extern filehandle_table_type fd_table;
extern int fd_next;

#endif /* __COMMON_HPP__ */

