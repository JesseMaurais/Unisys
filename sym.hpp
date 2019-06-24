#include "str.hpp"

namespace sys
{
	class sym
	{
		using string_view = fmt::string_view;
		using string = fmt::string;

	public:

		operator bool() const;
		sym();
		sym(string_view path);
		~sym();

		template <typename S> auto link(string_view name)
		{
			S *addr = nullptr;
			// see pubs.opengroup.org
			*(void**)(&addr) = link(name);
			return addr;
		}

	private:

		void *dl = nullptr;
		void *link(string_view name);
	};
}
