#define BOOST_TEST_MODULE ext2tests
// svn test

#include <boost/test/unit_test.hpp>
#include "host_node.hpp"
#include "../ext2_structs.hpp"
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

BOOST_AUTO_TEST_CASE(super_block_test) {
	ext2::detail::super_block firstblock;
	BOOST_REQUIRE_EQUAL(sizeof(firstblock), 1024);
}

BOOST_AUTO_TEST_CASE(write_read_superblock_test) {
	
	test_node d;
	ext2::super_block<test_node> test(d,0);

	test.flush();


}









