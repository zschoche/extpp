/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __EXT2_COMMON_HPP__
#define __EXT2_COMMON_HPP__

namespace ext2 {

namespace detail {



} /* namespace detail */


	enum error_code {
		e_block_overflow

	};
	
	struct ext2_error {
		const error_code code;
		ext2_error(error_code code) : code(code) {}
	};

} /* namespace ext2 */

#endif /* __EXT2_COMMON_HPP__ */
