#define BOOST_TEST_MODULE ext2tests
// svn test

#include <boost/test/unit_test.hpp>
#include "host_node.hpp"
#include "../ext2/filesystem.hpp"
#include "../ext2/block_device.hpp"
#include <fstream>
#include <iostream>

BOOST_AUTO_TEST_CASE(boost_test_test) { BOOST_REQUIRE_EQUAL(true, true); }

BOOST_AUTO_TEST_CASE(host_node_test) {
	std::remove("host_node_test_file");
	{
		std::fstream file("host_node_test_file", std::ios_base::binary | std::ios_base::in | std::ios_base::trunc | std::ios_base::out);
		BOOST_REQUIRE_EQUAL(file.is_open(), true);
		file << "Hello ext2!";
	}
	{
		host_node node("host_node_test_file", 11);
		BOOST_REQUIRE_EQUAL(node.length(), 11);
		char buffer[1024];
		BOOST_REQUIRE_EQUAL(node.read(6, buffer, 1024), 5);
		BOOST_CHECK(std::string(buffer, 5) == "ext2!");
		const char *test = "ASDFGH";
		BOOST_REQUIRE_EQUAL(node.write(3, test, 4), 4);
		BOOST_REQUIRE_EQUAL(node.read(0, buffer, 1024), 11);
		BOOST_CHECK(std::string(buffer, 11) == "HelASDFxt2!");
	}
	{
		std::fstream file("host_node_test_file", std::ios_base::binary | std::ios_base::in);
		BOOST_REQUIRE_EQUAL(file.is_open(), true);
		std::string s;
		file >> s;
		BOOST_CHECK(s == "HelASDFxt2!");
	}
}

BOOST_AUTO_TEST_CASE(superblock_test) {
	ext2::detail::superblock firstblock;
	BOOST_REQUIRE_EQUAL(sizeof(firstblock), 236);
	BOOST_REQUIRE_EQUAL(sizeof(firstblock), sizeof(ext2::detail::superblock));
}

BOOST_AUTO_TEST_CASE(group_descriptor_test) {
	ext2::detail::group_descriptor x;
	BOOST_REQUIRE_EQUAL(sizeof(x), 32);
}

BOOST_AUTO_TEST_CASE(inode_test) {
	ext2::detail::inode x;
	BOOST_REQUIRE_EQUAL(sizeof(x), 128);
}
BOOST_AUTO_TEST_CASE(os_spec_test) {
	using namespace ext2::detail;

	os::specific_value_1 x;
	BOOST_REQUIRE_EQUAL(sizeof(x), 4);
	os::linux::specific_value_2 l;
	os::hurd::specific_value_2 h;
	os::masix::specific_value_2 m;
	BOOST_REQUIRE_EQUAL(sizeof(l), sizeof(h));
	BOOST_REQUIRE_EQUAL(sizeof(h), sizeof(m));
	BOOST_REQUIRE_EQUAL(sizeof(h), 12);

}
struct test_device {

	char data[4096];

	test_device() {
		std::memset(data, 0, sizeof(data));
	}
	
	void write(uint64_t offset, const char* buffer, uint64_t size) {
		std::memcpy(data + offset, buffer, size);
	}

	void read(uint64_t offset, char* buffer, uint64_t size) {
		std::memcpy(buffer, data + offset, size);
	}

};


BOOST_AUTO_TEST_CASE(block_device_test) {

	ext2::block_device<test_device> d(5); // block have size of 5 bytes
	std::string test = "This is a test message.";
	d.write(0, test.c_str(), test.size()); 
	BOOST_CHECK(std::string(d.data) == test);
	char buffer[1024];
	d.read(0, buffer, test.size());
	BOOST_CHECK(std::string(buffer) == test);

	
	d.write(67, test.c_str(), test.size()); 
	BOOST_CHECK(std::string(&d.data[67]) == test);
	std::memset(buffer, 0, sizeof(buffer));
	d.read(67, buffer, test.size());
	BOOST_CHECK(std::string(buffer) == test);

	test = "asd";
	d.write(201, test.c_str(), test.size()); 
	BOOST_CHECK(std::string(&d.data[201]) == test);
	std::memset(buffer, 0, sizeof(buffer));
	d.read(201, buffer, test.size());
	BOOST_CHECK(std::string(buffer) == test);
}



BOOST_AUTO_TEST_CASE(read_superblock_test) {
	
	host_node image("image.img", 1024*1024*10);
	ext2::block_device<host_node> image2(128, "image.img", 1024*1024*10); // block have size of 128 bytes
	auto superblock = ext2::read_superblock(image);
	auto superblock2 = ext2::read_superblock(image2);
	BOOST_CHECK(superblock.data == superblock2.data);

	BOOST_REQUIRE_EQUAL(superblock.data.inode_count, 2560);
	BOOST_REQUIRE_EQUAL(superblock.data.block_count, 10240); 
	BOOST_REQUIRE_EQUAL(superblock.data.reserved_blocks_count, 512); 
	BOOST_REQUIRE_EQUAL(superblock.data.free_block_count, 9770); 
	BOOST_REQUIRE_EQUAL(superblock.data.free_inodes_count, 2540); 
	BOOST_REQUIRE_EQUAL(superblock.data.super_block_number, 1); 
	BOOST_REQUIRE_EQUAL(superblock.data.block_size_log, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.fragment_size_log, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.blocks_per_group, 8192);
	BOOST_REQUIRE_EQUAL(superblock.data.fragments_per_group, 8192); 
	BOOST_REQUIRE_EQUAL(superblock.data.inodes_per_group, 1280); 
	BOOST_REQUIRE_EQUAL(superblock.data.mount_count, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.mount_count_max, 65535); 
	BOOST_REQUIRE_EQUAL(superblock.data.ext2_magic_number, 61267); 
	BOOST_REQUIRE_EQUAL(superblock.data.file_system_state, 1); 
	BOOST_REQUIRE_EQUAL(superblock.data.error_behaviour, 1); 
	BOOST_REQUIRE_EQUAL(superblock.data.rev_level_minor, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.check_interval, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.os_id, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.rev_level_major, 1); 
	BOOST_REQUIRE_EQUAL(superblock.data.user_id_res_blocks, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.group_id_res_blocks, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.first_unreserved_inode, 11); 
	BOOST_REQUIRE_EQUAL(superblock.data.inode_size, 128); 
	BOOST_REQUIRE_EQUAL(superblock.data.this_partof_group, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.features_opt, 56); 
	BOOST_REQUIRE_EQUAL(superblock.data.features_req, 2); 
	BOOST_REQUIRE_EQUAL(superblock.data.features_readonly, 1); 
	BOOST_CHECK(superblock.data.volume_name() == ""); 
	BOOST_CHECK(superblock.data.mount_path_last() == "/home/pi/test"); 
	BOOST_REQUIRE_EQUAL(superblock.data.compression_algorithm, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.journal_inode, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.journal_device, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.orphan_list_head, 0); 
	BOOST_REQUIRE_EQUAL(superblock.data.block_group_count(), std::ceil((float)superblock.data.inode_count/superblock.data.inodes_per_group));
	BOOST_REQUIRE_EQUAL(superblock.data.block_group_count(), 2);
	BOOST_REQUIRE_EQUAL(superblock.data.block_size(), 1024); 
	BOOST_REQUIRE_EQUAL(superblock.data.fragment_size(), 1024); 
}

BOOST_AUTO_TEST_CASE(write_read_vector_test) {
	using block = ext2::block_data<test_device, char>;
	test_device d;
	std::string msg = "qwertzuiopasdfghjklyxcvbnm";
	d.write(0, msg.c_str(), msg.size());
	BOOST_CHECK(d.data == msg); 

	auto vec = ext2::read_vector<block>(d, 0, msg.size());
	BOOST_REQUIRE_EQUAL(vec.size(), msg.size()); 
	for(int i = 0; i < msg.size(); i++) {
		BOOST_REQUIRE_EQUAL(vec[i].data, msg[i]); 
	}

	for(auto& item : vec) {
		item.data = 'a';
	}
	ext2::write_vector(vec);
	for(int i = 0; i < msg.size(); i++) {
		BOOST_REQUIRE_EQUAL(d.data[i], 'a'); 
	}

}


BOOST_AUTO_TEST_CASE(read_group_des_table_test) {
	host_node image("image.img", 1024*1024*10);
	auto superblock = ext2::read_superblock(image);
	auto gd_table = ext2::read_group_descriptor_table(superblock);
	BOOST_REQUIRE_EQUAL(gd_table.size(), 2); 

	auto& gd1 =  gd_table[0];
	BOOST_REQUIRE_EQUAL(gd1.data.address_block_bitmap, 42);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_bitmap, 43);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_table, 44);
	BOOST_REQUIRE_EQUAL(gd1.data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gd1.data.free_inodes, 1262);
	BOOST_REQUIRE_EQUAL(gd1.data.count_directories, 4);

	auto& gd2 =  gd_table[1];
	BOOST_REQUIRE_EQUAL(gd2.data.address_block_bitmap, 8234);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_bitmap, 8235);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_table, 8236);
	BOOST_REQUIRE_EQUAL(gd2.data.free_blocks, 1842);
	BOOST_REQUIRE_EQUAL(gd2.data.free_inodes, 1278);
	BOOST_REQUIRE_EQUAL(gd2.data.count_directories, 2);

}


BOOST_AUTO_TEST_CASE(read_filesystem_root_test) {
	host_node image("image.img", 1024*1024*10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	BOOST_REQUIRE_EQUAL(root.data.type, 16877);
	BOOST_REQUIRE_EQUAL(root.data.uid, 1000);
	BOOST_REQUIRE_EQUAL(root.data.size, 1024);
	BOOST_REQUIRE_EQUAL(root.data.access_time_last, 1419087092);
	BOOST_REQUIRE_EQUAL(root.data.creation_time, 1419087091);
	BOOST_REQUIRE_EQUAL(root.data.mod_time, 1419087091);
	BOOST_REQUIRE_EQUAL(root.data.del_time, 0);
	BOOST_REQUIRE_EQUAL(root.data.gid, 1000);
	BOOST_REQUIRE_EQUAL(root.data.count_hard_link, 5);
	BOOST_REQUIRE_EQUAL(root.data.count_sector, 2);
	BOOST_REQUIRE_EQUAL(root.data.flags, 0);
	BOOST_REQUIRE_EQUAL(root.data.os_specific_1, 11);
	BOOST_REQUIRE_EQUAL(root.data.number_generation, 0);
	BOOST_REQUIRE_EQUAL(root.data.file_acl, 0);
	BOOST_REQUIRE_EQUAL(root.data.dir_acl, 0);
	BOOST_REQUIRE_EQUAL(root.data.fragment_address, 0);
	BOOST_REQUIRE_EQUAL(root.is_directory(),  true);
	BOOST_REQUIRE_EQUAL(root.is_regular_file(),  false);
	BOOST_REQUIRE_EQUAL(root.size(),  1024);
}
BOOST_AUTO_TEST_CASE(read_root_content_test) { //this is not done.
	host_node image("image.img", 1024*1024*10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	char* buffer = new char[root.size()];
	
	root.read(0, buffer, root.size());
	for(int i = 0; i < root.size(); i++) {
		std::cout << (int)buffer[i] << std::endl;
	}
	delete [] buffer;

}
