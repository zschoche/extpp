/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __EXT2_STRUCTS_HPP__
#define __EXT2_STRUCTS_HPP__

#include <boost/utility/string_ref.hpp>
#include <string>
#include <cmath>

namespace ext2 {
namespace detail {

// The different styles of the filesystem
enum file_system_states : uint16_t {
	file_system_clean = 1, // filesystem is clean
	file_system_error = 2  // filesytem has errors
};

template <typename OStream> OStream &operator<<(OStream &os, const file_system_states &state) {
	if (state == file_system_clean) {
		os << "clean";
	} else {
		os << "error";
	}
	os << "(" << static_cast<const uint32_t>(state) << ")";
	return os;
}

// error handling methods
enum error_handle_methods : uint16_t {
	error_handle_ignore = 1,	   // ignore the error
	error_handle_remount_readonly = 2, // remount file system as read-only
	error_handle_panic = 3		   // kernel panic
};

template <typename OStream> OStream &operator<<(OStream &os, const error_handle_methods &val) {
	if (val == error_handle_ignore) {
		os << "ignore";
	} else if (val == error_handle_remount_readonly) {
		os << "remount_readonly";
	} else {
		os << "panic";
	}
	os << "(" << static_cast<const uint32_t>(val) << ")";
	return os;
}
// creator operating system IDs
enum os_ids : uint32_t {
	os_linux = 0,   // Linux
	os_hurd = 1,    // GNU HURD
	os_masix = 2,   // MASIX
	os_freebsd = 3, // FreeBSD
	os_other = 4    // Other "Lites" BSD4.4-Lite derivatives such as NetBSD, OpenBSD, XNU/Darwin etc.
};
template <typename OStream> OStream &operator<<(OStream &os, const os_ids &val) {
	if (val == os_linux) {
		os << "Linux";
	} else if (val == os_hurd) {
		os << "GNU HURD";
	} else if (val == os_masix) {
		os << "MASIX";
	} else if (val == os_freebsd) {
		os << "FreeBSD";
	} else {
		os << "Other";
	}
	os << "(" << static_cast<const uint32_t>(val) << ")";
	return os;
}
enum opt_feature_flags : uint32_t {
	opt_feature_prealloc_blocks = 0x0001, // preallocate some number of (contiguous?) blocks (see byte 205 in the superblock) to a directory when creating a
					      // new one (to reduce fragmentation?)
	opt_feature_afs_inode_exist = 0x0002, // AFS server inodes exist
	opt_feature_has_journal = 0x0004,     // File system has a journal (Ext3)
	opt_feature_extended_inodes = 0x0008, // Inodes have extended attributes
	opt_feature_can_resize = 0x0010,      // File system can resize itself for larger partitions
	opt_feature_dir_use_hash = 0x0020     // directories use hash index
};
template <typename OStream> OStream &operator<<(OStream &os, const opt_feature_flags &val) {
	os << static_cast<const uint32_t>(val);
	return os;
}

enum req_feature_flags : uint32_t {
	req_feature_compressed = 0x0001,       // compression is used
	req_feature_dir_entries_type = 0x0002, // directory entries contain a type field
	req_feature_replay_journal = 0x0004,   // file system needs to replay its journal
	req_feature_uses_jornal = 0x0008       // file system uses a journal device
};
template <typename OStream> OStream &operator<<(OStream &os, const req_feature_flags &val) {
	os << static_cast<const uint32_t>(val);
	return os;
}

enum readonly_feature_flags : uint32_t {
	readonly_feature_sparse = 0x0001,      // sparse superblocks and group descripor tables
	readonly_feature_filesize_64 = 0x0002, // file sytem uses a 64 bit file size
	readonly_feature_bin_tree = 0x0004     // directory contents are stored in form of a Binary tree
};
template <typename OStream> OStream &operator<<(OStream &os, const readonly_feature_flags &val) {
	os << static_cast<const uint32_t>(val);
	return os;
}



namespace os {
enum specific_value_1 : uint32_t {
	LINUX, // TODO: whats are the values?
	HURD,
	MASIX
};

namespace linux {
struct __attribute__((packed)) specific_value_2 {
	uint8_t fragment_number; // fragment number
	uint8_t fragment_size;   // fragment size
	uint16_t padding;	// reserved
	uint16_t uid_high;       // high 16 bits of 32-bit user ID
	uint16_t gid_high;       // high 16 bits of 32-bit group ID
	uint32_t padding2;       // padding
};
} /* namespace linux */

namespace hurd {
struct __attribute__((packed)) specific_value_2 {
	uint8_t fragment_number; // fragment number
	uint8_t fragment_size;   // fragment size
	uint16_t type_high;      // high 16 bits of 32 bit "Type and Permissions" field
	uint16_t uid_high;       // high 16 bits of 32 bit user ID
	uint16_t gid_high;       // high 16 bits of 32 bit group ID
	uint32_t author_id;      // user ID of author (if == 0xFFFFFFFF, the normal user  ID will be used)
};
} /* namespace hurd */

namespace masix {
struct __attribute__((packed)) specific_value_2 {
	uint8_t fragment_number;
	uint8_t fragment_size;
	uint8_t res[10];
};
} /* namespace masix */

} /* namespace os */

enum inode_types : uint16_t {
	fifo = 0x1000,
	character_device = 0x2000,
	directory = 0x4000,
	block_device = 0x6000,
	regular_file = 0x8000,
	symbolic_link = 0xA000,
	unix_socket = 0xC000
};
enum inode_premissions : uint64_t {

	oexec = 0x001,	//Other—execute permission
	owrite = 0x002,	//Other—write permission
	oread = 0x004,	//Other—read permission

	gexec = 0x008,	//Group—execute permission
	gwrite = 0x010,	//Group—write permission
	gread = 0x020,	//Group—read permission

	uexec = 0x040,	//User—execute permission
	uwrite = 0x080,	//User—write permission
	uread = 0x100,	//User—read permission

	sticky_bit = 0x200, //Sticky Bit
	set_group_id = 0x400,	//Set group ID
	set_user_id = 0x800	//Set user ID


};

template <typename OStream> OStream &dump_inode_premissions(OStream &os, const uint16_t &val) {
	os << "other: ";
	if(val & oexec) os << 'x';
	if(val & owrite) os << 'w';
	if(val & oread) os << 'r';
	os << ", group: ";
	if(val & gexec) os << 'x';
	if(val & gwrite) os << 'w';
	if(val & gread) os << 'r';
	os << ", user: ";
	if(val & uexec) os << 'x';
	if(val & uwrite) os << 'w';
	if(val & uread) os << 'r';
	return os;
}

template <typename OStream> OStream &operator<<(OStream &os, const inode_types &val) {
	if (val & fifo) {
		os << "Fifo";
	} else if (val & character_device) {
		os << "Character Device";
	} else if (val & directory) {
		os << "Directory";
	} else if (val & block_device) {
		os << "Block Device";
	} else if (val & block_device) {
		os << "Block Device";
	} else if (val & regular_file) {
		os << "Regular File";
	} else if (val & symbolic_link) {
		os << "Symbolic Link";
	} else if (val & unix_socket) {
		os << "Unix Socket";
	} else {
		os << "Other";
	}
	os << "[";
	dump_inode_premissions(os, val);
	os << "]";
	os << "(" << static_cast<const uint32_t>(val) << ")";
	return os;
}

enum inode_flags : uint32_t {
	secure_deletion = 0x00000001,  /* Secure deletion (not used) */
	copy = 0x00000002,	     /* Keep a copy of data when deleted (not used) */
	compression = 0x00000004,      /* File compression (not used) */
	synchronous = 0x00000008,      /* Synchronous updates—new data is written immediately to disk */
	immutable = 0x00000010,	/* Immutable file (content cannot be changed) */
	append = 0x00000020,	   /* Append only */
	no_dump = 0x00000040,	  /* File is not included in 'dump' command */
	no_last_accessed = 0x00000080, /* Last accessed time should not updated */
	hash_dir = 0x00010000,	 /* Hash indexed directory */
	afs_dir = 0x00020000,	  /* AFS directory */
	journal = 0x00040000	   /* Journal file data */
};

/**
*
*/
struct __attribute__((packed)) inode {

	inode_types type; // type and permissions
	uint16_t uid;     // user ID

	uint32_t size;		   // lower 32 bits of size in bytes
	uint32_t access_time_last; // last access time (in posix time)
	uint32_t creation_time;    // creation time (in posix time)
	uint32_t mod_time;	 // last modification time (in posix time)
	uint32_t del_time;	 // last deletion time (in posix time)

	uint16_t gid;		  // group id
	uint16_t count_hard_link; // count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated

	uint32_t count_sector; // count of disc sectors (not ext2 blocks) in use by this inode, not counting the actual inode strucutre nor directory entries
			       // linking to the inode
	uint32_t flags;	// flags
	os::specific_value_1 os_specific_1; // operating system specific value #1

	uint32_t block_pointer_direct[12];  // direct block pointers 0-11
	uint32_t block_pointer_indirect[3]; // indirect block pointers (points to a block that is a list of block pointers to data)

	uint32_t number_generation; // generation number (primarily used for NFS)
	// the next to are reserved in version 0 of ext2. In version >= 1, they are used for File ACL and filesize (if file) / directory ACL (if directory)
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t fragment_address; // block address of fragment
	union {
		os::linux::specific_value_2 linux;
		os::hurd::specific_value_2 hurd;
		os::masix::specific_value_2 masix;
	} os_specific_2;

	template<typename OStream> OStream& dump(OStream& os) {
	os << "Inode Dump:\n";
	os << "\ttype: " << type << '\n';
	os << "\tuid: " << uid << '\n';
	os << "\tsize: " << size << '\n';
	os << "\taccess_time_last: " << access_time_last << '\n';
	os << "\tcreation_time: " << creation_time << '\n';
	os << "\tmod_time: " << mod_time << '\n';
	os << "\tdel_time: " << del_time << '\n';
	os << "\tgid: " << gid << '\n';
	os << "\tcount_hard_link: " << count_hard_link << '\n';
	os << "\tcount_sector: " << count_sector << '\n';
	os << "\tflags: " << flags << '\n';
	os << "\tos_specific_1: " << os_specific_1 << '\n';
	os << "\tnumber_generation: " << number_generation << '\n';
	os << "\tfile_acl: " << file_acl << '\n';
	os << "\tdir_acl: " << dir_acl << '\n';
	os << "\tfragment_address: " << fragment_address << '\n';
	 //os << "os_specific_2: " << os_specific_2 << '\n';
	os << "\tdata blocks:";
	for(auto id : block_pointer_direct) {
		if(id == 0) break;
		os << " " << id;
	}
	//	TODO: block_pointer_indirect[3]
	os << '\n';

		return os;
	}

};



/*
** based on wiki page wiki.osdev.org/Ext2
*/
struct __attribute__((packed)) superblock {
	uint32_t inode_count;		// total number of indes in the system
	uint32_t block_count;		// total number of blocks in the system
	uint32_t reserved_blocks_count; // number of blocks reserverd for superuser
	uint32_t free_block_count;      // total number of unallocated blocks
	uint32_t free_inodes_count;     // total number of unallocated inodes
	uint32_t super_block_number;    // block number of the block containing the superblock
	uint32_t block_size_log;	// log2 (block size) -10. (In other words, the number to shift 1024 to the left by to obtain the blcok size
	uint32_t fragment_size_log;     // log2 (fragment size) -10. (In other words, the number to shift 1024 to the let by to obtain the fragment size
	uint32_t blocks_per_group;      // Number of blocks in each block group
	uint32_t fragments_per_group;   // Number of fragments in each block group
	uint32_t inodes_per_group;      // NUmer of inodes in each block group
	uint32_t last_mount_time;       // last mount time in POSIX time
	uint32_t last_written_time;     // last written time in POSIX time

	uint16_t mount_count;		      // number of times the volume has been mounted since its last consistency check
	uint16_t mount_count_max;	     // number of moutns allowed before a consistency check (fsck) must be done
	uint16_t ext2_magic_number;	   // ext2 signature (0xef53) used to help confirm the presence of Ext2 on a volume
	file_system_states file_system_state; // state of the filesystem
	error_handle_methods error_behaviour; // tells what to do when error occurs
	uint16_t rev_level_minor;	     // the minor portion of the version (combine with major version to construct full field)

	uint32_t last_check;      // POSIX time of last consistency check
	uint32_t check_interval;  // the max time (in POSIX time)  between forced consistency checks
	os_ids os_id;		  // Operating system ID from which the filesystem on thsi volume was created
	uint32_t rev_level_major; // major portion of the version (combine with minor to construct full field)

	uint16_t user_id_res_blocks;  // User ID that can use reserved blocks
	uint16_t group_id_res_blocks; // groupp ID that can use reserved blocks

	/*
	* Extended Superblock fields
	* These fields are only present if Major version (specified in the base superlock fields), is greater than or equal to 1
	*/

	uint32_t first_unreserved_inode; // first non-reserved inode in file system (In versions <1.0, this is fixed as 11)
	uint16_t inode_size;		 // size of each inode structure in bytes
	uint16_t this_partof_group;      // block group that this superblock is part of (if backup copy)
	opt_feature_flags features_opt;  // optional features present (features that are not required to read or write , but usually result in a performance
					 // increase
	req_feature_flags features_req;  // required features present (features that are required to be supported to read or write
	readonly_feature_flags features_readonly; // features that if not supported, the volume must be mounted read-only //TODO make enum
	uint32_t file_system_id[4];		  // file system ID (what is output by blkid)
      private:
	char _volume_name[16];     // volume name (C-Style string: characters terminated by a 0 byte)
	char _mount_path_last[64]; // path volume was last mounted to (C-style string: characters terminated by a 0 byte)
      public:
	uint32_t compression_algorithm;  // compression algorithm used //TODO make enum
	uint8_t file_preallocate_blocks; // number of blcoks to preallocate for files
	uint8_t dir_preallocate_blocks;  // number of blcoks to preallocate for directories
	uint16_t padding_bytes1;	 // unused
	uint32_t journal_id[4];		 // journal ID, same style as the file system ID above
	uint32_t journal_inode;		 // journal inode
	uint32_t journal_device;	 // journal device
	uint32_t orphan_list_head;       // head of orphan inode lsit
	// TODO osdev.org says (unused) for bytes 236 to 1023 here, what to do?

	const boost::string_ref volume_name() const { return boost::string_ref(_volume_name); }

	const boost::string_ref mount_path_last() const { return boost::string_ref(_mount_path_last); }
	uint32_t block_group_count() const { return std::ceil(static_cast<float>(block_count) / static_cast<float>(blocks_per_group)); }
	uint32_t block_size() const { return 1024 << block_size_log; }
	uint32_t fragment_size() const { return 1024 << fragment_size_log; }

	bool large_files() const { return features_readonly & readonly_feature_filesize_64; }


	template <typename OStream> void dump(OStream &os) const {
		os << "Superblock Dump:\r\n";
		os << "\tinodes_count: " << inode_count << "\r\n";
		os << "\tblocks_count: " << block_count << "\r\n";
		os << "\treserved_blocks_count: " << reserved_blocks_count << "\r\n";
		os << "\tfree_block_count: " << free_block_count << "\r\n";
		os << "\tfree_inodes_count: " << free_inodes_count << "\r\n";
		os << "\tsuper_block_number: " << super_block_number << "\r\n";
		os << "\tblock_size_log: " << block_size_log << "\r\n";
		os << "\tfragment_size_log: " << fragment_size_log << "\r\n";
		os << "\tblocks_per_group: " << blocks_per_group << "\r\n";
		os << "\tfragments_per_group: " << fragments_per_group << "\r\n";
		os << "\tinodes_per_group: " << inodes_per_group << "\r\n";
		os << "\tlast_mount_time: " << last_mount_time << "\r\n";
		os << "\tlast_written_time: " << last_written_time << "\r\n";
		os << "\tmount_count: " << mount_count << "\r\n";
		os << "\tmount_count_max: " << mount_count_max << "\r\n";
		os << "\text2_magic_number: " << ext2_magic_number << "\r\n";
		os << "\tfile_system_state: " << file_system_state << "\r\n";
		os << "\terror_behaviour: " << error_behaviour << "\r\n";
		os << "\trev_level_minor: " << rev_level_minor << "\r\n";
		os << "\tlast_check: " << last_check << "\r\n";
		os << "\tcheck_interval: " << check_interval << "\r\n";
		os << "\tos_id: " << os_id << "\r\n";
		os << "\trev_level_major: " << rev_level_major << "\r\n";
		os << "\tuser_id_res_blocks: " << user_id_res_blocks << "\r\n";
		os << "\tgroup_id_res_blocks: " << group_id_res_blocks << "\r\n";
		os << "\tfirst_unreserved_inode: " << first_unreserved_inode << "\r\n";
		os << "\tinode_size: " << inode_size << "\r\n";
		os << "\tthis_partof_group: " << this_partof_group << "\r\n";
		os << "\tfeatures_opt: " << features_opt << "\r\n";
		os << "\tfeatures_req: " << features_req << "\r\n";
		os << "\tfeatures_readonly: " << features_readonly << "\r\n";
		os << "\tfile_system_id[4]: " << file_system_id << "\r\n";
		os << "\tvolume_name[16]: " << volume_name() << "\r\n";
		os << "\tmount_path_last[64]: " << mount_path_last() << "\r\n";
		os << "\tcompression_algorithm: " << compression_algorithm << "\r\n";
		os << "\tfile_preallocate_blocks: " << file_preallocate_blocks << "\r\n";
		os << "\tdir_preallocate_blocks: " << dir_preallocate_blocks << "\r\n";
		os << "\tpadding_bytes1: " << padding_bytes1 << "\r\n";
		os << "\tjournal_id[4]: " << journal_id << "\r\n";
		os << "\tjournal_inode: " << journal_inode << "\r\n";
		os << "\tjournal_device: " << journal_device << "\r\n";
		os << "\torphan_list_head: " << orphan_list_head << "\r\n";
		os << "\r\n";
	}
};

inline bool operator==(const superblock &lhs, const superblock &rhs) {
	return std::strncmp(reinterpret_cast<const char *>(&lhs), reinterpret_cast<const char *>(&rhs), sizeof(superblock)) == 0;
}

/**
* a block group descriptor contains information regarding where important data structures for that block group are located.
*	should be 32 byte long
*/
struct __attribute__((packed)) group_descriptor {
	uint32_t address_block_bitmap; // block address of block usage bitmap
	uint32_t address_inode_bitmap; // block adress of inode usage bitmap
	uint32_t address_inode_table;  // starting block address of inode table
	uint16_t free_blocks;	  // number of unallocated blocks in group
	uint16_t free_inodes;	  // number of unallocated inodes in group
	uint16_t count_directories;    // number of directories
	uint8_t unused[14];	    // padding bytes to get to 32 byte length

	template <typename OStream> OStream& dump(OStream &os) const {
		os << "Group Descriptor Dump:" << std::endl;
		os << "\taddress_block_bitmap: " << address_block_bitmap << std::endl;
		os << "\taddress_inode_bitmap: " << address_inode_bitmap << std::endl;
		os << "\taddress_inode_table: " << address_inode_table << std::endl;
		os << "\tfree_blocks: " << free_blocks << std::endl;
		os << "\tfree_inodes: " << free_inodes << std::endl;
		os << "\tcount_directorie: " << count_directories << std::endl;
		return os;
	}
};


} /* namespace detail */

} /* namespace ext2 */

#endif /* __EXT2_STRUCTS_HPP__ */
