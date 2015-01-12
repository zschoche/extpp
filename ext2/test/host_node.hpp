/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __HOST_NODE_HPP__
#define __HOST_NODE_HPP__

#include <fstream>
#include <string>

/*
 * this emulates a device
 * It's implements our Device Concept
 */
class host_node /* : public iposix::fs::i_fs_node*/ {

	mutable std::fstream file;
	const std::string filename;
	const size_t size;

	void open() const {
		file.close();
		file.open(filename.c_str(),std::ios_base::in | std::ios_base::out | std::ios_base::binary);
	}

	bool is_ok() const {
		return file.is_open() && file.good();
	}

	public:
	host_node(std::string filename,size_t size) :file(filename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary), filename(std::move(filename)), size(size) {
		
	
	}

	

	uint32_t read(const uint64_t offset, char *buffer, uint32_t length) const {
		if(!is_ok()) open();

		file.seekg(offset);
		file.read(buffer, length);
		auto r = file.gcount();
		file.flush();
		return r;
	}

	uint32_t write(const uint64_t offset, const char *buffer, uint32_t length) {
		if(!is_ok()) open();
		file.seekp(offset);
		file.write(buffer, length);
		file.flush();
		return length;
	}


	uint32_t length() const {
		return size;
	}



};

class test_node /* : public iposix::fs::i_fs_node*/ {


	size_t size;

	public:

	uint32_t read(const uint64_t offset, char *buffer, uint32_t length)  {
		return 0;
	}

	uint32_t write(const uint64_t offset, const char *buffer, uint32_t length) {
		std::cout << length << std::endl;
		return length;
	}


	uint32_t length() const {
		return size;
	}



};
#endif /* __HOST_NODE_HPP__ */
