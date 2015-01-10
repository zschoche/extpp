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

enum ops { explore, forward, cancel };

template <typename T, bool VISIT_DOT_AND_DOTDOT = false> struct visitor {

	inline T *derived() { return static_cast<T *>(this); }

	inline const T *derived() const { return static_cast<const T *>(this); }

	template <typename Inode> ops visit(Inode &inode) {

		ops result = explore;
		if (auto *dir = to_directory(&inode)) {
			auto list = dir->read_entrys();
			for (auto entry : list) {
				if (VISIT_DOT_AND_DOTDOT) {
					result = lookup(entry, inode);
				} else if (entry.name != "." && entry.name != "..") {
					result = lookup(entry, inode);
				}
				if (result == cancel)
					break;
			}
		}
		return result;
	}

	const path *get_current_path() const { return &_path; }

      private:
	path _path;

	template <typename Inode> ops lookup(detail::directory_entry &entry, Inode &inode) {
		ops result = explore;
		_path.push_back(std::make_pair(entry.inode_id, &entry.name));
		auto next = inode.get_fs()->get_inode(entry.inode_id);
		result = (*derived())(entry.name, &next);
		if (result == explore) {
			result = visit(next);
		}
		_path.pop_back();
		return result;
	}
};

template <typename OStream> struct printer : visitor<printer<OStream> > {

	printer(OStream &os) : os(os) {}
	template <typename Inode> ops operator()(const std::string &name, Inode *inode) {
		const ext2::path *p = this->get_current_path();
		os << *p;
		if (auto *symlink = to_symbolic_link(inode)) {
			os << " -> " << symlink->get_target();
		}
		os << '\n';
		return explore;
	}

      private:
	OStream &os;
};

struct finder : visitor<finder, true> {

	const std::vector<std::string> &unified_path;
	std::vector<std::string>::const_iterator pos;
	uint32_t inode_id = 0;

	finder(const std::vector<std::string> &unified_path) : unified_path(unified_path), pos(this->unified_path.cbegin()) {}
	template <typename Inode> ops operator()(const std::string &name, Inode *inode) {
		if (name == *pos) {
			pos++;
			if (pos == unified_path.end()) {
				inode_id = this->get_current_path()->back().first;
				return cancel;
			}
			return explore;
		}
		return forward;
	}
};

} /* namespace visitors */

template <typename OStream, typename Inode> OStream &print(OStream &os, Inode &inode) {

	visitors::printer<OStream> p(os);
	p.visit(inode);
	return os;
}

template <typename Inode> uint32_t find_inode(Inode &inode, const std::vector<std::string> &unified_path) {
	visitors::finder f(unified_path);
	f.visit(inode);
	return f.inode_id;
}

} /* namespace ext2 */

#endif /* __VISITORS_HPP__ */
