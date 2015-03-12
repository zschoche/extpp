#define BOOST_TEST_MODULE ext2tests
// svn test

#include <boost/test/unit_test.hpp>
#include "host_node.hpp"
#include "../ext2/filesystem.hpp"
#include "../ext2/block_device.hpp"
#include "../ext2/visitors.hpp"
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

	mutable char data[4096];

	test_device() { std::memset(data, 0, sizeof(data)); }

	void write(uint64_t offset, const char *buffer, uint64_t size) { std::memcpy(data + offset, buffer, size); }

	void read(uint64_t offset, char *buffer, uint64_t size) const { std::memcpy(buffer, data + offset, size); }
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

	host_node image("image.img", 1024 * 1024 * 10);
	ext2::block_device<host_node> image2(128, "image.img", 1024 * 1024 * 10); // block have size of 128 bytes
	auto superblock = ext2::read_superblock(image);
	auto superblock2 = ext2::read_superblock(image2);
	BOOST_CHECK(superblock.data == superblock2.data);

	BOOST_REQUIRE_EQUAL(superblock.data.inode_count, 2560);
	BOOST_REQUIRE_EQUAL(superblock.data.block_count, 10240);
	BOOST_REQUIRE_EQUAL(superblock.data.reserved_blocks_count, 512);
	BOOST_REQUIRE_EQUAL(superblock.data.free_block_count, 9770);
	BOOST_REQUIRE_EQUAL(superblock.data.free_inodes_count, 2536);
	BOOST_REQUIRE_EQUAL(superblock.data.super_block_number, 1);
	BOOST_REQUIRE_EQUAL(superblock.data.block_size_log, 0);
	BOOST_REQUIRE_EQUAL(superblock.data.fragment_size_log, 0);
	BOOST_REQUIRE_EQUAL(superblock.data.blocks_per_group, 8192);
	BOOST_REQUIRE_EQUAL(superblock.data.fragments_per_group, 8192);
	BOOST_REQUIRE_EQUAL(superblock.data.inodes_per_group, 1280);
	BOOST_REQUIRE_EQUAL(superblock.data.mount_count, 3);
	BOOST_REQUIRE_EQUAL(superblock.data.mount_count_max, 65535);
	BOOST_REQUIRE_EQUAL(superblock.data.ext2_magic_number, 61267);
	BOOST_REQUIRE_EQUAL(superblock.data.file_system_state, 0); // filesyste was mounted
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
	BOOST_REQUIRE_EQUAL(superblock.data.block_group_count(), std::ceil((float)superblock.data.inode_count / superblock.data.inodes_per_group));
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
	for (auto i = 0u; i < msg.size(); i++) {
		BOOST_REQUIRE_EQUAL(vec[i].data, msg[i]);
	}

	for (auto &item : vec) {
		item.data = 'a';
	}
	ext2::write_vector(vec);
	for (auto i = 0u; i < msg.size(); i++) {
		BOOST_REQUIRE_EQUAL(d.data[i], 'a');
	}
}

BOOST_AUTO_TEST_CASE(read_group_des_table_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto superblock = ext2::read_superblock(image);
	auto gd_table = ext2::read_group_descriptor_table(superblock);
	BOOST_REQUIRE_EQUAL(gd_table.size(), 2);

	auto &gd1 = gd_table[0];
	BOOST_REQUIRE_EQUAL(gd1.data.address_block_bitmap, 42);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_bitmap, 43);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_table, 44);
	BOOST_REQUIRE_EQUAL(gd1.data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gd1.data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gd1.data.count_directories, 4);

	auto &gd2 = gd_table[1];
	BOOST_REQUIRE_EQUAL(gd2.data.address_block_bitmap, 8234);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_bitmap, 8235);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_table, 8236);
	BOOST_REQUIRE_EQUAL(gd2.data.free_blocks, 1842);
	BOOST_REQUIRE_EQUAL(gd2.data.free_inodes, 1278);
	BOOST_REQUIRE_EQUAL(gd2.data.count_directories, 2);
}

BOOST_AUTO_TEST_CASE(read_filesystem_root_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	BOOST_REQUIRE_EQUAL(root.data.type, 16877);
	BOOST_REQUIRE_EQUAL(root.data.uid, 1000);
	BOOST_REQUIRE_EQUAL(root.data.size, 1024);
	BOOST_REQUIRE_EQUAL(root.data.access_time_last, 1420835878);
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
	BOOST_REQUIRE_EQUAL(root.is_directory(), true);
	BOOST_REQUIRE_EQUAL(root.is_regular_file(), false);
	BOOST_REQUIRE_EQUAL(root.size(), 1024);
}

BOOST_AUTO_TEST_CASE(inode_size_test) {
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::directory<ext2::filesystem<host_node> >));
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::file<ext2::filesystem<host_node> >));
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::character_device<ext2::filesystem<host_node> >));
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::block_device<ext2::filesystem<host_node> >));
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::fifo<ext2::filesystem<host_node> >));
	BOOST_REQUIRE_EQUAL(sizeof(ext2::inode<ext2::filesystem<host_node> >), sizeof(ext2::inodes::symbolic_link<ext2::filesystem<host_node> >));
	BOOST_CHECK(sizeof(ext2::detail::directory_entry) > 8);
}

BOOST_AUTO_TEST_CASE(read_root_content_test) {
	host_node image("image.img", 1024 * 1024 * 10);

	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	BOOST_REQUIRE_EQUAL(root.get_inode_block_id(), 44);
	if (auto *dir = ext2::to_directory(&root)) {
		ext2::directory_entry_list list = dir->read_entrys();
		/*
		for(auto& item : list) {
			std::cout << item.name << std::endl;
		}
		*/

		BOOST_REQUIRE_EQUAL(list.size(), 6);
		BOOST_REQUIRE_EQUAL(list[0].inode_id, 2);
		BOOST_REQUIRE_EQUAL(list[0].size, 12);
		BOOST_CHECK(list[0].name == ".");

		BOOST_REQUIRE_EQUAL(list[1].inode_id, 2);
		BOOST_REQUIRE_EQUAL(list[1].size, 12);
		BOOST_CHECK(list[1].name == "..");

		BOOST_REQUIRE_EQUAL(list[2].inode_id, 11);
		BOOST_REQUIRE_EQUAL(list[2].size, 20);
		BOOST_CHECK(list[2].name == "lost+found");

		BOOST_REQUIRE_EQUAL(list[3].inode_id, 1281);
		BOOST_REQUIRE_EQUAL(list[3].size, 12);
		BOOST_CHECK(list[3].name == "tmp");

		BOOST_REQUIRE_EQUAL(list[4].inode_id, 1282);
		BOOST_REQUIRE_EQUAL(list[4].size, 12);
		BOOST_CHECK(list[4].name == "tmp2");

		BOOST_REQUIRE_EQUAL(list[5].inode_id, 13);
		BOOST_REQUIRE_EQUAL(list[5].size, 956);
		BOOST_CHECK(list[5].name == "testfile");
	} else {
		BOOST_ERROR("that should be a directory");
	}
}
BOOST_AUTO_TEST_CASE(read_testfile_test) {
	host_node image("image.img", 1024 * 1024 * 10);

	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	if (auto *dir = ext2::to_directory(&root)) {
		auto entrys = dir->read_entrys();

		auto testfile = std::find_if(entrys.begin(), entrys.end(), [](auto &item) -> bool { return item.name == "testfile"; });
		BOOST_CHECK(testfile != entrys.end());

		auto inode = filesystem.get_inode(testfile->inode_id);
		if (auto *file = ext2::to_file(&inode)) {
			std::stringstream ss;
			ss << *file;
			BOOST_CHECK(ss.str() == "This is a test file.\n");
		} else {
			BOOST_ERROR("that should be a file");
		}
	} else {
		BOOST_ERROR("that should be a directory");
	}
}



BOOST_AUTO_TEST_CASE(visitor_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	std::stringstream ss;
	ext2::print(ss, root);
	std::string str = "/lost+found\n"
		"/tmp\n"
		"/tmp/testdir\n"
		"/tmp/testdir/largefile2\n"
		"/tmp/testdir/largefile\n"
		"/tmp2\n"
		"/tmp2/testdir\n"
		"/tmp2/testdir/largefile2\n"
		"/tmp2/testdir/largefile\n"
		"/tmp2/testdir/link -> ../../testfile\n"
		"/tmp2/testdir/tmp -> ../../tmp\n"
		"/tmp2/testdir/tmp2_loop -> ../../tmp2\n"
		"/tmp2/testdir/largefile_with_more_than_60_chars_01234567890123456789012345678901234567890123456789012345678901234567890123456789 -> largefile\n"
		"/testfile\n";
	std::cout << ss.str() << std::endl;
	BOOST_CHECK(ss.str() == str);



}

BOOST_AUTO_TEST_CASE(print_fs_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	filesystem.dump(std::cout);
	
	std::cout << "\n\n ### Content ###" << std::endl;
	ext2::print(std::cout, root);

}
//*/

BOOST_AUTO_TEST_CASE(path_1_test) {
	std::string str = "/tmp2/testdir/largefile";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { "tmp2", "testdir", "largefile" };
	BOOST_REQUIRE_EQUAL(p.str, str);
	BOOST_REQUIRE_EQUAL(p.vec.size(), 3);
	for(auto i = 0u; i < v.size(); i++) {
		BOOST_CHECK(v[i] == p.vec[i]);

	}
}
BOOST_AUTO_TEST_CASE(path_2_test) {
	std::string str = "tmp2/testdir/largefile";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { "tmp2", "testdir", "largefile" };
	BOOST_REQUIRE_EQUAL(p.str, str);
	BOOST_REQUIRE_EQUAL(p.vec.size(), 3);
	for(auto i = 0u; i < v.size(); i++) {
		BOOST_CHECK(v[i] == p.vec[i]);

	}
}
BOOST_AUTO_TEST_CASE(path_3_test) {
	std::string str = "tmp2/testdir/largefile/";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { "tmp2", "testdir", "largefile" };
	BOOST_REQUIRE_EQUAL(p.str, str);
	BOOST_REQUIRE_EQUAL(p.vec.size(), 3);
	for(auto i = 0u; i < v.size(); i++) {
		BOOST_CHECK(v[i] == p.vec[i]);

	}
}

BOOST_AUTO_TEST_CASE(path_4_test) {
	std::string str = "/tmp2/testdir/largefile/";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { "tmp2", "testdir", "largefile" };
	BOOST_REQUIRE_EQUAL(p.str, str);
	BOOST_REQUIRE_EQUAL(p.vec.size(), 3);
	for(auto i = 0u; i < v.size(); i++) {
		BOOST_CHECK(v[i] == p.vec[i]);

	}
}

BOOST_AUTO_TEST_CASE(path_5_test) {
	std::string str = "/";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { };
	BOOST_REQUIRE_EQUAL(p.vec.size(), 0);
}

BOOST_AUTO_TEST_CASE(path_6_test) {
	std::string str = "///////";
	ext2::path p = ext2::path_from_string(str);
	std::vector<std::string> v = { };
	BOOST_REQUIRE_EQUAL(p.vec.size(), 0);
}

BOOST_AUTO_TEST_CASE(find_file_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/testdir/largefile");
	BOOST_REQUIRE_EQUAL(inode_id, 18);
	
}

BOOST_AUTO_TEST_CASE(read_big_file_test) {

	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/testdir/largefile");
	BOOST_REQUIRE_EQUAL(inode_id, 18);

	auto inode = filesystem.get_inode(inode_id);
	std::string str;
	//672 lines
	for(int i = 0; i < 672; i++) {
		str += "a bit more content.\n";
	}
	//str.erase(str.size(), 1);
	if(auto* file = ext2::to_file(&inode)) {
		std::stringstream ss;
		ss << *file;
		BOOST_CHECK(ss.str() == str);

	} else {
		BOOST_ERROR("that should be a file");
	}
}

BOOST_AUTO_TEST_CASE(read_symlink_1_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/testdir/link", false);
	BOOST_REQUIRE_EQUAL(inode_id, 19);
	auto inode = filesystem.get_inode(inode_id);
	auto* symlink = ext2::to_symbolic_link(&inode);
	BOOST_CHECK(symlink != nullptr);
	std::string target = symlink->get_target();
	BOOST_CHECK(target == "../../testfile");
}

BOOST_AUTO_TEST_CASE(read_symlink_2_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/testdir/largefile_with_more_than_60_chars_01234567890123456789012345678901234567890123456789012345678901234567890123456789", false);
	BOOST_REQUIRE_EQUAL(inode_id, 22);
	auto inode = filesystem.get_inode(inode_id);
	auto* symlink = ext2::to_symbolic_link(&inode);
	BOOST_CHECK(symlink != nullptr);
	std::string target = symlink->get_target();
	BOOST_CHECK(target == "largefile");
}

BOOST_AUTO_TEST_CASE(find_file_2_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/../tmp2/./testdir/largefile");
	BOOST_REQUIRE_EQUAL(inode_id, 18);
	
}

BOOST_AUTO_TEST_CASE(find_file_3_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/../tmp2////./testdir///largefile");
	BOOST_REQUIRE_EQUAL(inode_id, 18);
	
}
BOOST_AUTO_TEST_CASE(find_file_symlink_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/../tmp2/./testdir/link");
	BOOST_REQUIRE_EQUAL(inode_id, 13);
	
}

BOOST_AUTO_TEST_CASE(find_file_symlink_2_test) {
	host_node image("image.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t inode_id = ext2::find_inode(root, "/tmp2/testdir/tmp/testdir/largefile");
	BOOST_REQUIRE_EQUAL(inode_id, 15);
	
}

BOOST_AUTO_TEST_CASE(bitmap_test) {

	ext2::bitmap<test_device> b(nullptr, 0, 16);
	BOOST_REQUIRE_EQUAL(b.get(0), false);
	BOOST_REQUIRE_EQUAL(b.find(true, 0), -1);
	b.set(0, true);
	BOOST_REQUIRE_EQUAL(b.get(0), true);
	BOOST_REQUIRE_EQUAL(b.find(true, 0), 0);
	BOOST_REQUIRE_EQUAL(b.find(true, 1), 0);
	BOOST_REQUIRE_EQUAL(b.find(true, 2), 0);
	

}

BOOST_AUTO_TEST_CASE(alloc_block_test) {
	std::remove("alloc_block_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("alloc_block_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("alloc_block_test.img", 1024 * 1024 * 10);

	auto sb = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb.data.free_block_count, 9770);
	auto gdt = ext2::read_group_descriptor_table(sb);
	BOOST_REQUIRE_EQUAL(gdt[0].data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gdt[1].data.free_blocks, 1842);

	auto filesystem = ext2::read_filesystem(image);
	auto id = filesystem.alloc_block();


	BOOST_REQUIRE_EQUAL(id, 221);

	auto sb1 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb1.data.free_block_count, 9769);
	auto gd_table = ext2::read_group_descriptor_table(sb1);
	BOOST_REQUIRE_EQUAL(gd_table[0].data.free_blocks, 7927);
	BOOST_REQUIRE_EQUAL(gd_table[1].data.free_blocks, 1842);

	filesystem.free_block(id);

	auto sb2 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb2.data.free_block_count, 9770);

	auto gd_table1 = ext2::read_group_descriptor_table(sb2);
	BOOST_REQUIRE_EQUAL(gd_table1[0].data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gd_table1[1].data.free_blocks, 1842);

	std::remove("alloc_block_test.img");
}
BOOST_AUTO_TEST_CASE(alloc_block_all_test) {
	std::remove("alloc_block_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("alloc_block_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("alloc_block_test.img", 1024 * 1024 * 10);

	auto sb = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb.data.free_block_count, 9770);


	auto filesystem = ext2::read_filesystem(image);
	auto gdt = ext2::read_group_descriptor_table(sb);
	BOOST_REQUIRE_EQUAL(gdt[0].data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gdt[1].data.free_blocks, 1842);
	std::vector<uint32_t> blocks;
	auto count = sb.data.free_block_count;
	for(auto i = 0u; i < count; i++) {
		blocks.push_back(filesystem.alloc_block());
	}
	sb.load();
	BOOST_REQUIRE_EQUAL(sb.data.free_block_count, 0);

	for(auto& id : blocks) {
		filesystem.free_block(id);
	}

	auto sb2 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb2.data.free_block_count, 9770);

	auto gd_table1 = ext2::read_group_descriptor_table(sb2);
	BOOST_REQUIRE_EQUAL(gd_table1[1].data.free_blocks, 1842);
	BOOST_REQUIRE_EQUAL(gd_table1[0].data.free_blocks, 7928);

	std::remove("alloc_block_test.img");
}

BOOST_AUTO_TEST_CASE(alloc_inode_test) {
	std::remove("alloc_inode_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("alloc_inode_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("alloc_inode_test.img", 1024 * 1024 * 10);

	auto sb = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb.data.free_inodes_count, 2536);
	auto gdt = ext2::read_group_descriptor_table(sb);
	BOOST_REQUIRE_EQUAL(gdt[0].data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gdt[1].data.free_inodes, 1278);

	auto filesystem = ext2::read_filesystem(image);
	auto id = filesystem.alloc_inode();


	BOOST_REQUIRE_EQUAL(id, 23);

	auto sb1 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb1.data.free_inodes_count, 2535);
	auto gd_table = ext2::read_group_descriptor_table(sb1);
	BOOST_REQUIRE_EQUAL(gd_table[0].data.free_inodes, 1257);
	BOOST_REQUIRE_EQUAL(gd_table[1].data.free_inodes, 1278);

	filesystem.free_inode(id);

	auto sb2 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb2.data.free_inodes_count, 2536);

	auto gd_table1 = ext2::read_group_descriptor_table(sb2);
	BOOST_REQUIRE_EQUAL(gd_table1[0].data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gd_table1[1].data.free_inodes, 1278);

	std::remove("alloc_inode_test.img");
}
BOOST_AUTO_TEST_CASE(alloc_inode_all_test) {
	std::remove("alloc_inode_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("alloc_inode_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("alloc_inode_test.img", 1024 * 1024 * 10);

	auto sb = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb.data.free_inodes_count, 2536);


	auto filesystem = ext2::read_filesystem(image);
	auto gdt = ext2::read_group_descriptor_table(sb);
	BOOST_REQUIRE_EQUAL(gdt[0].data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gdt[1].data.free_inodes, 1278);
	std::vector<uint32_t> blocks;
	auto count = sb.data.free_inodes_count;
	for(auto i = 0u; i < count; i++) {
		blocks.push_back(filesystem.alloc_inode());
	}
	sb.load();
	BOOST_REQUIRE_EQUAL(sb.data.free_inodes_count, 0);
	BOOST_REQUIRE_EQUAL(blocks.size(), count);

	for(auto& id : blocks) {
		filesystem.free_inode(id);
	}

	auto sb2 = ext2::read_superblock(image);
	BOOST_REQUIRE_EQUAL(sb2.data.free_inodes_count, 2536);

	auto gd_table1 = ext2::read_group_descriptor_table(sb2);
	BOOST_REQUIRE_EQUAL(gd_table1[1].data.free_inodes, 1278);
	BOOST_REQUIRE_EQUAL(gd_table1[0].data.free_inodes, 1258);

	std::remove("alloc_inode_test.img");
}



BOOST_AUTO_TEST_CASE(change_file_test) {
	std::remove("change_file_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("change_file_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("change_file_test.img", 1024 * 1024 * 10);

	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t id = ext2::find_inode(root, "/testfile");
	auto inode = filesystem.get_inode(id);
	auto* file = ext2::to_file(&inode);
	BOOST_CHECK(file != nullptr);
	std::string msg = "NEW";
	file->write(4, msg.c_str(), msg.size());
	std::stringstream ss;
	ss << *file;
	BOOST_CHECK(ss.str() == "ThisNEW a test file.\n");
	std::remove("change_file_test.img");
}



BOOST_AUTO_TEST_CASE(ext_file_test) {
	std::remove("ext_file_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_file_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_file_test.img", 1024 * 1024 * 10);

	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	uint32_t id = ext2::find_inode(root, "/testfile");
	auto inode = filesystem.get_inode(id);
	auto* file = ext2::to_file(&inode);
	BOOST_CHECK(file != nullptr);
	std::string msg = " file will need more than one block of data: 1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm1234567890qwertzuiopasdfghjklyxcvbnm";
	BOOST_CHECK(msg.size() > 1024);
	file->write(4, msg.c_str(), msg.size());

	auto filesystem2= ext2::read_filesystem(image);
	auto root2 = filesystem2.get_root();

	uint32_t id2 = ext2::find_inode(root2, "/testfile");
	auto inode2 = filesystem2.get_inode(id2);
	auto* file2 = ext2::to_file(&inode2);
	BOOST_CHECK(file2 != nullptr);

	std::stringstream ss;
	ss << *file2;
	std::stringstream ss2;
	ss2 << *file;
	BOOST_CHECK(ss.str() == "This" + msg);
	BOOST_CHECK(ss2.str() == "This" + msg);
	BOOST_REQUIRE_EQUAL(file->size(), msg.size() + 4);
	BOOST_REQUIRE_EQUAL(file2->size(), msg.size() + 4);


	//resize again
	file->set_size(10);

	auto filesystem3 = ext2::read_filesystem(image);
	auto root3 = filesystem3.get_root();

	uint32_t id3 = ext2::find_inode(root3, "/testfile");
	auto inode3 = filesystem3.get_inode(id3);
	auto* file3 = ext2::to_file(&inode3);
	BOOST_CHECK(file3 != nullptr);

	std::stringstream ss3;
	ss3 << *file3;
	std::stringstream ss4;
	ss4 << *file;
	BOOST_CHECK(ss3.str() == "This file ");
	BOOST_CHECK(ss4.str() == "This file ");
	BOOST_REQUIRE_EQUAL(file3->size(), 10);
	BOOST_REQUIRE_EQUAL(file->size(), 10);


	std::remove("ext_file_test.img");
}


BOOST_AUTO_TEST_CASE(ext_create_file_test) {
	std::remove("ext_create_file_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_create_file_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_create_file_test.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	auto id_file = filesystem.create_file();
	if(auto* dir = ext2::to_directory(&root)) {
		auto entry = ext2::create_directory_entry("new_file", id_file.first, id_file.second);
		*dir << entry;
		auto entrys = dir->read_entrys();
		BOOST_CHECK(std::find_if(entrys.begin(), entrys.end(),[&] (auto& e) { return e.inode_id == id_file.first && e.name == "new_file"; }) != entrys.end());

	} else {
		BOOST_ERROR("root is not a directory");
	}
	std::remove("ext_create_file_test.img");
}

BOOST_AUTO_TEST_CASE(ext_create_symlink_test) {
	std::remove("ext_create_symlink_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_create_symlink_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_create_symlink_test.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	auto id_file = filesystem.create_symbolic_link("./testfile");
	if(auto* dir = ext2::to_directory(&root)) {
		auto entry = ext2::create_directory_entry("symlink", id_file.first, id_file.second);
		*dir << entry;
		auto entrys = dir->read_entrys();
		BOOST_CHECK(std::find_if(entrys.begin(), entrys.end(),[&] (auto& e) { return e.inode_id == id_file.first && e.name == "symlink"; }) != entrys.end());
		root.load();
		uint32_t id1 = ext2::find_inode(root, "/symlink");
		uint32_t id2 = ext2::find_inode(root, "/testfile");
		BOOST_REQUIRE_EQUAL(id1, id2);
		BOOST_REQUIRE_EQUAL(id1, 13);

	} else {
		BOOST_ERROR("root is not a directory");
	}
	std::remove("ext_create_symlink_test.img");
}

BOOST_AUTO_TEST_CASE(ext_create_symlink_2_test) {
	std::remove("ext_create_symlink_2_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_create_symlink_2_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_create_symlink_2_test.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	auto id_file = filesystem.create_symbolic_link("/tmp2/testdir/largefile_with_more_than_60_chars_01234567890123456789012345678901234567890123456789012345678901234567890123456789");
	if(auto* dir = ext2::to_directory(&root)) {
		auto entry = ext2::create_directory_entry("symlink", id_file.first, id_file.second);
		*dir << entry;
		auto entrys = dir->read_entrys();
		BOOST_CHECK(std::find_if(entrys.begin(), entrys.end(),[&] (auto& e) { return e.inode_id == id_file.first && e.name == "symlink"; }) != entrys.end());
		root.load();
		uint32_t id1 = ext2::find_inode(root, "/symlink");
		uint32_t id2 = ext2::find_inode(root, "/tmp2/testdir/largefile_with_more_than_60_chars_01234567890123456789012345678901234567890123456789012345678901234567890123456789");
		BOOST_REQUIRE_EQUAL(id1, id2);
		BOOST_REQUIRE_EQUAL(id1, 18);

	} else {
		BOOST_ERROR("root is not a directory");
	}
	std::remove("ext_create_symlink_2_test.img");
}
BOOST_AUTO_TEST_CASE(ext_create_dir_test) {
	std::remove("ext_create_dir_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_create_dir_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_create_dir_test.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();
	auto id_dir = filesystem.create_directory(2);
	if(auto* dir = ext2::to_directory(&root)) {
		auto entry = ext2::create_directory_entry("new_dir", id_dir.first, id_dir.second);
		*dir << entry;
		auto entrys = dir->read_entrys();
		BOOST_CHECK(std::find_if(entrys.begin(), entrys.end(),[&] (auto& e) { return e.inode_id == id_dir.first && e.name == "new_dir"; }) != entrys.end());
		root.load();
		uint32_t id1 = ext2::find_inode(root, "/new_dir");
		uint32_t id2 = ext2::find_inode(root, "/new_dir/.");
		BOOST_REQUIRE_EQUAL(id1, id2);
		BOOST_REQUIRE_EQUAL(id1, id_dir.first);
		BOOST_REQUIRE_EQUAL(ext2::find_inode(root, "/new_dir/.."), 2);

		auto inode = filesystem.get_inode(id1);
		if(auto* dir = ext2::to_directory(&inode)) {
			auto entrys = dir->read_entrys();
			BOOST_REQUIRE_EQUAL(entrys.size(), 2);
			BOOST_CHECK(entrys[0].name == ".");
			BOOST_CHECK(entrys[0].inode_id == id_dir.first);
			BOOST_CHECK(entrys[1].name == "..");
			BOOST_CHECK(entrys[1].inode_id == 2);

		} else {
			BOOST_ERROR("this is not a directory");
		}
	} else {
		BOOST_ERROR("root is not a directory");
	}
	std::remove("ext_create_dir_test.img");
}
BOOST_AUTO_TEST_CASE(ext_check_backup_test) {
	std::remove("ext_check_backup_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("ext_check_backup_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("ext_check_backup_test.img", 1024 * 1024 * 10);
	{
	auto prim = ext2::read_superblock(image);
	auto backup = ext2::read_superblock(image, 8193*1024);
	BOOST_CHECK(prim.data == backup.data);
	auto gd_table = ext2::read_group_descriptor_table(prim);
	auto gd_table_backup = ext2::read_group_descriptor_table(backup);
	BOOST_REQUIRE_EQUAL(gd_table.size(), 2);
	BOOST_REQUIRE_EQUAL(gd_table_backup.size(), 2);
	BOOST_CHECK(gd_table[0].data.free_blocks != gd_table_backup[0].data.free_blocks); // the backup is currently not in sync
	BOOST_CHECK(gd_table[0].data.free_inodes != gd_table_backup[0].data.free_inodes); // the backup is currently not in sync
	BOOST_CHECK(gd_table[1].data.free_blocks != gd_table_backup[1].data.free_blocks); // the backup is currently not in sync
	BOOST_CHECK(gd_table[1].data.free_inodes != gd_table_backup[1].data.free_inodes); // the backup is currently not in sync
	for(auto i = 0u; i < gd_table.size(); i++) {
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_block_bitmap, gd_table_backup[i].data.address_block_bitmap);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_inode_bitmap, gd_table_backup[i].data.address_inode_bitmap);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_inode_table, gd_table_backup[i].data.address_inode_table);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.count_directories, gd_table[i].data.count_directories);
	}
	auto &gd1 = gd_table[0];
	BOOST_REQUIRE_EQUAL(gd1.data.address_block_bitmap, 42);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_bitmap, 43);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_table, 44);
	BOOST_REQUIRE_EQUAL(gd1.data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gd1.data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gd1.data.count_directories, 4);

	auto &gd2 = gd_table[1];
	BOOST_REQUIRE_EQUAL(gd2.data.address_block_bitmap, 8234);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_bitmap, 8235);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_table, 8236);
	BOOST_REQUIRE_EQUAL(gd2.data.free_blocks, 1842);
	BOOST_REQUIRE_EQUAL(gd2.data.free_inodes, 1278);
	BOOST_REQUIRE_EQUAL(gd2.data.count_directories, 2);
	}
	auto filesystem = ext2::read_filesystem(image);
	filesystem.write_superblock_backup();
	{
	auto prim = ext2::read_superblock(image);
	auto backup = ext2::read_superblock(image, 8193*1024);
	BOOST_CHECK(prim.data == backup.data);
	auto gd_table = ext2::read_group_descriptor_table(prim);
	auto gd_table_backup = ext2::read_group_descriptor_table(backup);
	BOOST_REQUIRE_EQUAL(gd_table.size(), 2);
	BOOST_REQUIRE_EQUAL(gd_table_backup.size(), 2);
	BOOST_CHECK(gd_table[0].data.free_blocks == gd_table_backup[0].data.free_blocks); // the backup is now in sync
	BOOST_CHECK(gd_table[0].data.free_inodes == gd_table_backup[0].data.free_inodes); // the backup is now in sync
	BOOST_CHECK(gd_table[1].data.free_blocks == gd_table_backup[1].data.free_blocks); // the backup is now in sync
	BOOST_CHECK(gd_table[1].data.free_inodes == gd_table_backup[1].data.free_inodes); // the backup is now in sync
	for(auto i = 0u; i < gd_table.size(); i++) {
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_block_bitmap, gd_table_backup[i].data.address_block_bitmap);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_inode_bitmap, gd_table_backup[i].data.address_inode_bitmap);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.address_inode_table, gd_table_backup[i].data.address_inode_table);
		BOOST_REQUIRE_EQUAL(gd_table[i].data.count_directories, gd_table[i].data.count_directories);
	}
	auto &gd1 = gd_table[0];
	BOOST_REQUIRE_EQUAL(gd1.data.address_block_bitmap, 42);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_bitmap, 43);
	BOOST_REQUIRE_EQUAL(gd1.data.address_inode_table, 44);
	BOOST_REQUIRE_EQUAL(gd1.data.free_blocks, 7928);
	BOOST_REQUIRE_EQUAL(gd1.data.free_inodes, 1258);
	BOOST_REQUIRE_EQUAL(gd1.data.count_directories, 4);

	auto &gd2 = gd_table[1];
	BOOST_REQUIRE_EQUAL(gd2.data.address_block_bitmap, 8234);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_bitmap, 8235);
	BOOST_REQUIRE_EQUAL(gd2.data.address_inode_table, 8236);
	BOOST_REQUIRE_EQUAL(gd2.data.free_blocks, 1842);
	BOOST_REQUIRE_EQUAL(gd2.data.free_inodes, 1278);
	BOOST_REQUIRE_EQUAL(gd2.data.count_directories, 2);
	}
	std::remove("ext_check_backup_test.img");
}

BOOST_AUTO_TEST_CASE(remove_test) {
	std::remove("remove_test.img");
	{
		std::ifstream source("image.img", std::ios::binary);
    		std::ofstream dest("remove_test.img", std::ios::binary);
		std::istreambuf_iterator<char> begin_source(source);
		std::istreambuf_iterator<char> end_source;
		std::ostreambuf_iterator<char> begin_dest(dest); 
		std::copy(begin_source, end_source, begin_dest);
	}
	host_node image("remove_test.img", 1024 * 1024 * 10);
	auto filesystem = ext2::read_filesystem(image);
	auto root = filesystem.get_root();

	auto tmp2id = ext2::find_inode(root, "/tmp2");	
	auto inode = filesystem.get_inode(tmp2id);
	auto* tmp2 = ext2::to_directory(&inode);
	BOOST_CHECK(tmp2 != nullptr);
	BOOST_REQUIRE_EQUAL(tmp2->remove("testdir"), false); //testdir is not empty
	auto entrys = tmp2->read_entrys();
	auto testdirid = std::find_if(entrys.begin(), entrys.end(), [](auto& e) { return e.name == "testdir"; })->inode_id;
	BOOST_REQUIRE_EQUAL(testdirid, 16); 
	auto testdir_inode = filesystem.get_inode(testdirid);
	auto* testdir = ext2::to_directory(&testdir_inode);
	BOOST_CHECK(testdir!= nullptr);
	BOOST_REQUIRE_EQUAL(testdir->remove("."), false); // that is not possible
	BOOST_REQUIRE_EQUAL(testdir->remove(".."), false); // that is not possible
	BOOST_REQUIRE_EQUAL(testdir->remove("largefile2"), true); 
	BOOST_REQUIRE_EQUAL(testdir->remove("largefile"), true); 
	BOOST_REQUIRE_EQUAL(testdir->remove("link"), true); 
	BOOST_REQUIRE_EQUAL(testdir->remove("tmp"), true); 
	BOOST_REQUIRE_EQUAL(testdir->remove("tmp2_loop"), true); 
	BOOST_REQUIRE_EQUAL(testdir->remove("largefile_with_more_than_60_chars_01234567890123456789012345678901234567890123456789012345678901234567890123456789"), true); 
	BOOST_REQUIRE_EQUAL(tmp2->remove("testdir"), true); //testdir is empty now
	BOOST_REQUIRE_EQUAL(ext2::find_inode(root, "/tmp2/testdir"), 0); 

	std::remove("remove_test.img");
}
