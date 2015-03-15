/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __VISITORS_HPP__
#define __VISITORS_HPP__

#include "filesystem.hpp"

namespace ext2 {

typedef std::vector<std::pair<uint64_t, std::string *> > inode_path;
template <typename OStream> OStream &operator<<(OStream &os, const inode_path &p) {
	for (const auto &item : p) {
		os << '/' << *item.second;
	}
	return os;
}

namespace visitors {

enum ops { explore, forward, cancel };

template <typename T, bool VISIT_DOT_AND_DOTDOT = false> struct visitor {

	inline T *derived() { return static_cast<T *>(this); }

	inline const T *derived() const { return static_cast<const T *>(this); }

	template <typename Inode> ops visit(const Inode &inode) {

		ops result = explore;
		if (const auto *dir = to_directory(&inode)) {
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

	const inode_path *get_current_path() const { return &_path; }

      private:
	inode_path _path;

	template <typename Inode> ops lookup(detail::directory_entry &entry, Inode &inode) {
		ops result = explore;
		_path.push_back(std::make_pair(entry.inode_id, &entry.name));
		auto next = inode.fs()->get_inode(entry.inode_id);
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
		const ext2::inode_path *p = this->get_current_path();
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

template<typename Filesystem>
struct finder : visitor<finder<Filesystem>, true> {

	typedef Filesystem fs_type;
	typedef typename fs_type::inode_type inode_type;
	path target;
	path::vec_type::const_iterator pos;
	bool hide_symlink = true;
	uint32_t inode_id = 0;

	finder(path target) : target(std::move(target)), pos(this->target.vec.cbegin()) {}

	ops operator()(const std::string &name, inode_type *inode) {
		if (name == *pos) {
			pos++;
			if(auto* dir = to_directory(inode)) {
				cur_dir = dir;
			}

			if (hide_symlink || pos != target.vec.cend()) {
			if (auto *symlink = to_symbolic_link(inode) ) { // follow
				explore_symlink(symlink);
				return cancel;
			}
			}
			if (pos == target.vec.cend()) {
				inode_id = this->get_current_path()->back().first;
				return cancel;
			}
			return explore;
		}
		return forward;
	}

      private:
	inode_type* cur_dir = nullptr;
	void explore_symlink(typename fs_type::symbolic_link_type *link) {
		std::string path_str = link->get_target();
		if(path_str.back() != '/') 
			path_str += '/';

		for(auto iter = pos; iter != target.vec.end(); iter++) {
			path_str += *iter;
			path_str += '/';
		}

		
		path p = path_from_string(path_str);
		finder f(p);
		if (p.is_relative() && cur_dir != nullptr) {
			f.visit(*cur_dir);
		} else {
			auto root = link->fs()->get_root();
			f.visit(root);
		}
		inode_id = f.inode_id;
	}
};

} /* namespace visitors */

template <typename OStream, typename Inode> OStream &print(OStream &os, const Inode& inode) {

	visitors::printer<OStream> p(os);
	p.visit(inode);
	return os;
}

template <typename Inode> uint32_t find_inode(Inode &inode, const ext2::path &path, bool hide_symlink = true) {
	visitors::finder<typename Inode::fs_type> f(path);
	f.hide_symlink = hide_symlink;
	f.visit(inode);
	return f.inode_id;
}
template <typename Inode> uint32_t find_inode(Inode &inode, const std::string &path, bool hide_symlink = true) {
	return find_inode(inode, path_from_string(path), hide_symlink);
}

} /* namespace ext2 */

#endif /* __VISITORS_HPP__ */
