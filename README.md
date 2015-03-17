## extpp // ext2 implementaion, header-only, C++14

I've written this ext2 implementation  during the winter semester 2014/2015. It was my final project in a bare bone operating system lecture.

Please be lenient with me I have implemented it short before the [hell week](http://www.urbandictionary.com/define.php?term=Hell+Week). Therefore, inline documentation is really rare. I will give a short introduction below.

### Introduction
First of all, There is a concept of an device:

```cpp
struct device {
	void write(uint64_t offset, const char* buffer, uint64_t size);
	void read(uint64_t offset, const char* buffer, uint64_t size);
};
```
This is what you have to provide. Wherever you ext2 image is, you have to make it available with something that have the described ``write()`` and ``read()`` method. If it is a block device, then may have a look into the ext2/block_device.hpp

Afterward, there is a ``read_filesystem(Device& d,...)`` method in ext2/filesystem.hpp which returns the file system from the given device. Below, we call that file system ``fs``.

Each object in this file system has an idea where it belongs to and provides a ``load()`` and ``save()``method. It follows a write-through philosophy. Thus, it is assumed that nobody else is writing on that device.

Good starting point is ``fs.get_root()`` which returns the inode of ``/``.
The inode is defined in ext2/inode.hpp. Apropos, if you want to access the underlying data of any data object, like an inode, use this ``data``attribute. 

The ``ext2::inode`` provides basic functionalities which every inode has.
If you want to do something with a specific inode type, then you have to cast your inode into that specific inode type befor you can do it. Helper functions for theses casts are provided by this library. Here is an example to read the content of the root node:

```cpp
your_device d;
//...
auto fs = ext2::read_filesystem(d);
auto root = fs.get_root();
if(auto* dir = ext2::to_directory(&root)) {
	auto entries = dir->read_entries();
	for(const auto& e : entries) {
		std::cout << e.name << std::endl;
	} 
} else {
	/* root is not a directory */
}
```
Please notice that the variable ``root`` describes the life cycle of our inode.



### Issues
- Our bare bone operation system has no idea what an user is, therefore we do not have a elegant way to check user privileges. 
- We still need unit tests for really large files.
- Provide a device encapsulation with caching functionalities.
- Modify ``ext2::inode::write()`` to do copy-on-write.
- Some ext2 device types are not explicit implemented:
 - fifo
 - unix_socket
 - (block_device)
 - (character device)
- Everything in ./etools and ./osxfuse-ext2 is almost completely untested.
