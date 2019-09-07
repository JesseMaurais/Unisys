#ifndef uni_mman_hpp
#define uni_mman_hpp

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "uni.hpp"
#include "ptr.hpp"
#include "err.hpp"

namespace sys::uni
{
	inline auto make_map(size_t sz, int prot, int flags, int fd, off_t off = 0, void* ptr = nullptr)
	{
		ptr = mmap(ptr, sz, prot, flags, fd, off);
		if (MAP_FAILED == ptr)
		{
			sys::err(here, "mmap", ptr, sz, off);
			ptr = nullptr;
		}

		return make_ptr(ptr, [sz](auto ptr)
		{
			if (nullptr != ptr)
			{
				if (fail(munmap(ptr, sz)))
				{
					sys::err(here, "munmap", ptr, sz);
				}
			}
		});
	}

	class shm
	{
		char const *str;

	public:

		shm(char const *name) : str(name)
		{ }

		~shm()
		{
			if (fail(shm_unlink(str)))
			{
				err(here, "shm_unlink", str);
			}
		}

		int open(int flags, mode_t mode)
		{
			return shm_open(str, flags, mode);
		}
	};
}

#endif // file