/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __VISITORS_HPP__
#define __VISITORS_HPP__

#include "filesystem.hpp"

namespace ext2 {

namespace visitors {

enum ops {
	explore,
	forward,
	cancel
};

template <typename T> struct visitor {

	inline T *derived() { return static_cast<T *>(this); }

	inline const T *derived() const { return static_cast<const T *>(this); }

	template <typename Inode> ops visit(Inode& inode) {

		// do not follow symbolic link cycle possibility
		ops result = explore;
		if (!inode.is_symbolic_link()) {
			if (auto *dir = to_directory(&inode)) {
				auto list = dir->read_entrys();
				for (auto entry : list) {
					if (entry.name != "." && entry.name != "..") {
						_path.push_back(std::make_pair(entry.inode_id, &entry.name));
						auto next = inode.get_fs()->get_inode(entry.inode_id);
						result = (*derived())(entry.name, &next);
						if (result == explore) {
							result = visit(next);
						}
						if(result == cancel) break;
						_path.pop_back();
					}
				}
			}
		}
		return result;
	}

	const path* get_current_path() const {
		return &_path;
		
	}

      private:
	path _path;
};

template<typename OStream>
struct printer : visitor<printer<OStream>> {

	printer(OStream& os) : os(os) {}
	template <typename Inode> ops operator()(const std::string &name, Inode *) {
		const ext2::path* p = this->get_current_path();
		os <<  *p << '\n';
		return explore;
	}

	private:
	OStream& os;
};


struct finder : visitor<finder> {
	
	const std::vector<std::string>& unified_path;
	std::vector<std::string>::const_iterator pos;
	uint32_t inode_id = 0;

	finder(const std::vector<std::string>& unified_path) : unified_path(unified_path), pos(this->unified_path.cbegin()) {}
	template <typename Inode> ops operator()(const std::string &name, Inode *inode) {
		if(name == *pos) {
			std::cout << name << std::endl;
			pos++;
			if(pos == unified_path.end()) {
				inode_id = this->get_current_path()->back().first;
				return cancel;

			} 
			return explore;
		} 
		return forward;
	}



};

} /* namespace visitors */

template<typename OStream, typename Inode>
OStream& print(OStream& os, Inode& inode) {
	
	visitors::printer<OStream> p(os);
	p.visit(inode);
	return os;
}

template<typename Inode>
uint32_t find_inode(Inode& inode, const std::vector<std::string>& unified_path) {
	visitors::finder f(unified_path);
	f.visit(inode);
	return f.inode_id;
}

} /* namespace ext2 */


#endif /* __VISITORS_HPP__ */

