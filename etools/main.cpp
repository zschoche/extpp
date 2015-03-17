#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "../ext2/filesystem.hpp"
#include <iostream>
#include <iterator>
#include "../test/host_node.hpp"
#include "../ext2/filesystem.hpp"
#include "../ext2/visitors.hpp"

namespace po = boost::program_options;
namespace bfs = boost::filesystem;

template <typename Dir> void copy_file(Dir *dir, const bfs::path &source) {
	if(!bfs::exists(source)) {
		std::cerr << source << " not found.\n";
		exit(1);
	}
	if(!bfs::is_regular_file(source)) {
		std::cerr << source << " is not a file.\n";
		exit(1);
	}

	std::string filename = source.filename().string();
	auto entrys = dir->read_entrys();
	if(ext2::find_entry_by_name(entrys, filename) != entrys.end()) {
		std::cerr << filename << " is already in the image.\n";
		exit(1);
	}

	bfs::ifstream is(source, std::ios::in | std::ios::binary);
	if (is.is_open()) {
		auto id_file = dir->fs()->create_file();
		ext2::detail::device_stream<decltype(id_file.second)> os(&id_file.second);
		std::array<char, 1024> buffer;
		while (!is.read(buffer.data(), buffer.size()).eof()) {
			os.write(buffer.data(), buffer.size());
		}
		if (is.gcount() > 0) {
			os.write(buffer.data(), is.gcount());
		}
		auto entry = ext2::create_directory_entry(filename, id_file.first, id_file.second);
		*dir << entry;
	} else {
		std::cerr << "error: could not open " << source << std::endl;
		exit(1);
	}
}

template <typename Dir> void copy_to_image(uint32_t inode_id, Dir *target_dir, const bfs::path &source) {

	bfs::directory_iterator end_iter;
	for (bfs::directory_iterator dir_iter(source); dir_iter != end_iter; ++dir_iter) {

		if (bfs::is_symlink(*dir_iter)) {
			auto entrys = target_dir->read_entrys();
			auto iter = ext2::find_entry_by_name(entrys, dir_iter->path().filename().string());
			if (iter != entrys.end()) {
				std::cout << "delete old version of: " << dir_iter->path() << std::endl;
				if (!target_dir->fs()->get_inode(iter->inode_id).is_symbolic_link()) {
					std::cerr << dir_iter->path() << " is not a file." << std::endl;
				}
				target_dir->remove(iter->name);
			}
			std::cout << "creating: " << dir_iter->path() << std::endl;
			bfs::path target = bfs::read_symlink(*dir_iter);
			auto id_file = target_dir->fs()->create_symbolic_link(target.string());
			auto entry = ext2::create_directory_entry(dir_iter->path().filename().string(), id_file.first, id_file.second);
			*target_dir << entry;

		} else if (bfs::is_directory(dir_iter->status())) {
			auto entrys = target_dir->read_entrys();
			auto iter = ext2::find_entry_by_name(entrys, dir_iter->path().filename().string());
			if (iter != entrys.end()) {
				std::cout << "found: " << dir_iter->path() << std::endl;
				auto inode = target_dir->fs()->get_inode(iter->inode_id);
				if (auto *d = ext2::to_directory(&inode)) {
					copy_to_image(iter->inode_id, d, dir_iter->path());
				} else {
					std::cerr << dir_iter->path() << " is not a directory." << std::endl;
					exit(1);
				}
			} else {
				std::cout << "creating: " << dir_iter->path() << std::endl;
				auto id_dir = target_dir->fs()->create_directory(inode_id);
				auto entry = ext2::create_directory_entry(dir_iter->path().filename().string(), id_dir.first, id_dir.second);
				*target_dir << entry;
				if (auto *d = ext2::to_directory(&id_dir.second)) {
					copy_to_image(id_dir.first, d, dir_iter->path());
				}
			}

		} else if (bfs::is_regular_file(dir_iter->status())) {
			auto entrys = target_dir->read_entrys();
			auto iter = ext2::find_entry_by_name(entrys, dir_iter->path().filename().string());
			if (iter != entrys.end()) {
				std::cout << "delete old version of: " << dir_iter->path() << std::endl;
				if (!target_dir->fs()->get_inode(iter->inode_id).is_regular_file()) {
					std::cerr << dir_iter->path() << " is not a file." << std::endl;
					exit(1);
				}
				target_dir->remove(iter->name);
			}
			bfs::ifstream is(dir_iter->path(), std::ios::in | std::ios::binary);
			if (is.is_open()) {
				std::cout << "creating: " << dir_iter->path() << std::endl;
				auto id_file = target_dir->fs()->create_file();
				ext2::detail::device_stream<decltype(id_file.second)> os(&id_file.second);
				std::array<char, 1024> buffer;
				while (!is.read(buffer.data(), buffer.size()).eof()) {
					os.write(buffer.data(), buffer.size());
				}
				if (is.gcount() > 0) {
					os.write(buffer.data(), is.gcount());
				}

				auto entry = ext2::create_directory_entry(dir_iter->path().filename().string(), id_file.first, id_file.second);
				*target_dir << entry;
			} else {
				std::cerr << "warning: could not open " << dir_iter->path() << std::endl;
			}
		}
	}
}

int main(int ac, char *av[]) {
	try {
		// int uid;
		// int gid;
		int offset;
		po::options_description desc("etools can write into an ext2 image.\nIMPORTENT: All privileges and flags on "
				"the host system are ignored.\n\n Options");
		desc.add_options()("help", "produce help message")
				("image,i", po::value<std::string>(), "the ext2 image")
				("offset", po::value<int>(&offset)->default_value(0), "ext2 partition offset in bytes")
				("root,r", po::value<std::string>(), "the root directory which will be copied into the image")
				("mkdir", po::value<std::string>(), "creates a directory")
				("target-dir,t", po::value<std::string>(), "target direcotry for copy-files")
				("read-file", po::value<std::string>(), "prints a file to cout")
				("dump", "creates a directory")
				("copy-files,c", po::value<std::vector<std::string>>()->composing(), "copys a list of files into target-dir");
		/*("uid", po::value<int>(&uid)->default_value(0), "uid for all entrys")
		("gid", po::value<int>(&gid)->default_value(0), "gid for all entrys");*/

		po::positional_options_description p;
		p.add("copy-files", -1);
		
		po::variables_map vm;
		//po::store(po::parse_command_line(ac, av, desc), vm);
		//po::notify(vm);

		po::store(po::command_line_parser(ac, av).
				          options(desc).positional(p).run(), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 1;
		}

		if (vm.count("image")) {
			bfs::path image(vm["image"].as<std::string>());
			if (!bfs::exists(image)) {
				std::cerr << image << " does not exists.\n";
				return 1;
			}
			if (bfs::is_directory(image)) {
				std::cerr << image << " is a directory.\n";
				return 1;
			}
			host_node proxy(image.string(), bfs::file_size(image));
			auto filesystem = ext2::read_filesystem(proxy, offset);
			if (!filesystem.is_magic_number_ok()) {
				std::cerr << image << " that is not a ext2 filesystem image.\n";
				return 1;
			}

			if (vm.count("mkdir")) {
				std::string path_str = vm["mkdir"].as<std::string>();
				auto index = path_str.find_last_of("/");
				if (index == std::string::npos) {
					std::cerr << "error: need a absolut path\n";
					return 1;
				}
				std::string path = path_str.substr(0, index + 1);
				std::string dirname = path_str.substr(index + 1);

				auto root = filesystem.get_root();
				uint32_t inodeid = 2;
				if(path != "/") {
					inodeid = ext2::find_inode(root, path);
					if (inodeid == 0) {
						std::cerr << path << " not found.\n";
						return 1;
					}
				}
				auto inode = filesystem.get_inode(inodeid);
				if (auto *d = ext2::to_directory(&inode)) {
					auto entrys = d->read_entrys();
					if (ext2::find_entry_by_name(entrys, dirname) != entrys.end()) {
						std::cerr << path_str << " already exists.\n";
						return 1;
					}
					auto id_dir = filesystem.create_directory(inodeid);
					auto entry = ext2::create_directory_entry(dirname, id_dir.first, id_dir.second);
					*d << entry;
					std::cout << "created: " << path_str << std::endl;
				} else {
					std::cerr << path << " is not a directory.\n";
					return 1;
				}
			}
			if (vm.count("root")) {
				bfs::path dir(vm["root"].as<std::string>());
				if (!bfs::exists(dir)) {
					std::cerr << dir << " does not exists.\n";
					return 1;
				}
				if (!bfs::is_directory(dir)) {
					std::cerr << image << " is not a directory.\n";
					return 1;
				}
				std::cout << dir << " => " << image << "\n";
				auto r = filesystem.get_root();
				if (auto *d = ext2::to_directory(&r)) {
					copy_to_image(2, d, dir); // 2 is always the inode id of "/"

				} else {
					std::cerr << "Error on reading root direcotry in " << image << std::endl;
				}
			}
			if(vm.count("copy-files")) {
				if(vm.count("target-dir")) {
					std::string path = vm["target-dir"].as<std::string>(); 

					auto root = filesystem.get_root();
					uint32_t inodeid = 2;
					if(path != "/") {
						inodeid = ext2::find_inode(root, path);
						if (inodeid == 0) {
							std::cerr << path << " not found.\n";
							return 1;
						}
					}
					auto inode = filesystem.get_inode(inodeid);
					if(auto* d = ext2::to_directory(&inode)) {	
						
						std::vector<std::string> files = vm["copy-files"].as<std::vector<std::string>>();
						for(const auto& file : files) {
							std::cout << "copy: " << file << std::endl;
							copy_file(d, file);
						}
					} else {
						std::cerr << "error: " << path << " is not a directory.\n";
						return 1;
					}
				} else {
					std::cerr << "error: no target dir.\n";
					return 1;
				}
			}
			if (vm.count("dump")) {
				filesystem.dump(std::cout);
				std::cout << "\n\nContent:\n\n";
				ext2::print(std::cout, filesystem.get_root());
			}


			if(vm.count("read-file")) {
				std::string path = vm["read-file"].as<std::string>(); 
				auto root = filesystem.get_root();
				auto inodeid = ext2::find_inode(root, path);
				if(inodeid == 0) {
					std::cerr << path << " not found.\n";
					return 1;
				}
				auto inode = filesystem.get_inode(inodeid);
				if(auto* file = ext2::to_file(&inode)) {
					std::cout << *file;

				} else {
					std::cerr << path << " is not a file.\n";
				}
			}
		} else {
			std::cerr << "ext2 image was not set.\n";
		}
	} catch (std::exception &e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	} catch (...) {
		std::cerr << "Exception of unknown type!\n";
	}

	return 0;
}
