/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __EXT2_STUCTS_HPP__
#define __EXT2_STUCTS_HPP__

namespace ext2 {
namespace detail {
	template<typename T> write_to_disk(const T& value) {


	};

// TODO: create the basic stucts and enums here

/*
** based on wiki page wiki.osdev.org/Ext2
*/
struct super_block{
	uint32_t inodes_count; //total number of indes in the system
	uint32_t blocks_count; //total number of blocks in the system
	uint32_t reserved_blocks_count; //number of blocks reserverd for superuser
	uint32_t free_block_count; //total number of unallocated blocks
	uint32_t free_inodes_count; //total number of unallocated inodes
	uint32_t super_block_number; //block number of the block containing the superblock
	uint32_t block_size_log; //log2 (block size) -10. (In other words, the number to shift 1024 to the left by to obtain the blcok size
	uint32_t fragment_size_log; //log2 (fragment size) -10. (In other words, the number to shift 1024 to the let by to obtain the fragment size
	uint32_t blocks_per_group; //Number of blocks in each block group
	uint32_t fragments_per_group; //Number of fragments in each block group
	uint32_t inodes_per_group; //NUmer of inodes in each block group
	uint32_t last_mount_time; //last mount time in POSIX time
	uint32_t last_written_time; //last written time in POSIX time
	uint16_t mount_count; //number of times the volume has been mounted since its last consistency check
	uint16_t mount_count_max; //number of moutns allowed before a consistency check (fsck) must be done
	uint16_t ext2_magic_number; //ext2 signature (0xef53) used to help confirm the presence of Ext2 on a volume
	uint16_t file_system_state; //state of the filesystem //TODO make enum
	uint16_t error_behaviour; //tells what to do when error occurs //TODO make enum
	uint16_t rev_level_minor; //the minor portion of the version (combine with major version to construct full field)
	uint32_t last_check; //POSIX time of last consistency check
	uint32_t check_interval; //the max time (in POSIX time)  between forced consistency checks
	uint32_t os_id; //Operating system ID from which the filesystem on thsi volume was created
	uint32_t rev_level_major; //major portion of the version (combine with minor to construct full field)
	uint16_t user_id_res_blocks; //User ID that can use reserved blocks
	uint16_t group_id_res_blocks; //groupp ID that can use reserved blocks

	/*
	* Extended Superblock fields
	* These fields are only present if Major version (specified in the base superlock fields), is greater than or equal to 1
	*/

	uint32_t first_unreserved_inode; //first non-reserved inode in file system (In versions <1.0, this is fixed as 11)
	uint16_t inode_size; //size of each inode structure in bytes
	uint16_t this_partof_group; //block group that this superblock is part of (if backup copy)
	uint32_t features_opt; //optional features present (features that are not required to read or write , but usually result in a performance increase //TODO make enum
	uint32_t features_req; //required features present (features that are required to be supported to read or write //TODO make enum
	uint32_t features_readonly; //features that if not supported, the volume must be mounted read-only //TODO make enum
	uint32_t file_system_id[4]; //file system ID (what is output by blkid)
	char volume_name[16]; //volume name (C-Style string: characters terminated by a 0 byte)
	char mount_path_last[64]; //path volume was last mounted to (C-style string: characters terminated by a 0 byte)
	uint32_t compression_algorithm; //compression algorithm used //TODO make enum
	uint8_t file_preallocate_blocks; //number of blcoks to preallocate for files
	uint8_t dir_preallocate_blocks; //number of blcoks to preallocate for directories
	uint16_t padding_bytes1; //unused
	uint32_t journal_id[4]; //journal ID, same style as the file system ID above
	uint32_t journal_inode; //journal inode
	uint32_t journal_device; //journal device
	uint32_t orphan_list_head; //head of orphan inode lsit
	//TODO osdev.org says (unused) for bytes 236 to 1023 here, what to do? 
	
};

//The different styles of the filesystem
enum file_system_states{
	file_system_clean = 1,			//filesystem is clean
	file_system_error = 2			//filesytem has errors
};

//error handling methods
enum error_handle_methods{
	error_handle_ignore = 1,		//ignore the error
	error_handle_remount_readonly = 2,	//remount file system as read-only
	error_handle_panic = 3			//kernel panic
};

//creator operating system IDs
enum os_id{
	os_linux = 0,				//Linux
	os_hurd = 1,				//GNU HURD
	os_masix = 2,				//MASIX
	os_freebsd = 3,				//FreeBSD
	os_other = 4				//Other "Lites" BSD4.4-Lite derivatives such as NetBSD, OpenBSD, XNU/Darwin etc.
};

enum opt_feature_flags{
	opt_feature_prealloc_blocks = 0x0001,	//preallocate some number of (contiguous?) blocks (see byte 205 in the superblock) to a directory when creating a new one (to reduce fragmentation?)
	opt_feature_afs_inode_exist = 0x0002,	//AFS server inodes exist
	opt_feature_has_journal = 0x0004,	//File system has a journal (Ext3)
	opt_feature_extended_inodes = 0x0008,	//Inodes have extended attributes
	opt_feature_can_resize = 0x0010,	//File system can resize itself for larger partitions
	opt_feature_dir_use_hash = 0x0020	//directories use hash index
};

enum req_feature_flags{
	req_feature_compressed = 0x0001,	//compression is used
	req_feature_dir_entries_type = 0x0002,	//directory entries contain a type field
	req_feature_replay_journal = 0x0004,	//file system needs to replay its journal
	req_feature_uses_jornal = 0x0008	//file system uses a journal device
};

enum readonly_feature_flags{
	readonly_feature_sparse = 0x0001,	//sparse superblocks and group descripor tables
	readonly_feature_filesize_64 = 0x0002,	//file sytem uses a 64 bit file size
	readonly_feature_bin_tree = 0x0004	//directory contents are stored in form of a Binary tree
};




} /* namespace detail */
} /* namespace ext2 */

#endif /* __EXT2_STUCTS_HPP__ */
