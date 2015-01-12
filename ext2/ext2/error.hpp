/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <stdexcept>
namespace ext2 {
	
	namespace error {
		struct no_free_block_error : std::runtime_error {
			no_free_block_error() : std::runtime_error("no_free_block_error") {}
		};
		struct no_free_inode_error : std::runtime_error {
			no_free_inode_error() : std::runtime_error("no_free_inode_error") {}
		};
		
	} /* namespace error */

} /* namespace ext2 */


#endif /* __ERROR_HPP__ */

