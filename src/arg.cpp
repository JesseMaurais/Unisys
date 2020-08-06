// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "arg.hpp"
#include "ini.hpp"
#include "str.hpp"
#include "type.hpp"
#include "dir.hpp"
#include "usr.hpp"
#include "sys.hpp"
#include "sync.hpp"
#include <iterator>
#include <algorithm>

namespace
{
	fmt::string::view::vector list;

	auto make_key()
	{
		static auto const app = fmt::str::put("Application");
		return app;
	}

	auto make_pair(env::opt::word key = make_key())
	{
		static auto const cmd = fmt::str::set("Command Line");
		return std::make_pair(cmd, key);
	}

	auto make_ini()
	{
		return fmt::join({env::opt::program(), "ini"}, ".");
	}

	auto& registry()
	{
		static sys::exclusive<doc::ini> ini;
		// try read
		{
			auto const reader = ini.read();
			if (not empty(reader->keys))
			{
				return ini;
			}
		}
		// next write
		{
			auto writer = ini.write();
			auto const path = env::opt::initials();
			auto const s = fmt::to_string(path);
			doc::ini::ref slice = *writer;
			slice.set(make_pair(), env::opt::program());
			std::ifstream input(s);
			while (input >> slice);
			return ini;
		}
	}

	auto find_next(char** argv, env::opt::commands cmd)
	{
		auto const begin = cmd.begin();
		auto const end = cmd.end();

		auto next = end;
		unsigned argn = 0;

		while (argv[argn] and end == next)
		{
			fmt::string::view argu(argv[argn]);

			++argn;

			if (argu.starts_with("--"))
			{
				auto const entry = argu.substr(2);
				next = std::find_if
				(
					begin, end, [entry](auto const& d)
					{
						return d.name == entry;
					}
				);
			}
			else
			if (argu.starts_with("-"))
			{
				auto const entry = argu.substr(1);
				next = std::find_if
				(
					begin, end, [entry](auto const& d)
					{
						return d.dash == entry;
					}
				);
			}
		}
		return std::pair { argn, next };
	}
}

namespace env::opt
{
	fmt::string::view application()
	{
		return env::opt::get(make_key());
	}

	fmt::string::view::span arguments()
	{
		return { list.data(), list.size() };
	}

	fmt::string::view initials()
	{
		static fmt::string s;
		if (empty(s))
		{
			s = fmt::dir::join({config(), make_ini()});
		}
		return s;
	}

	fmt::string::view program()
	{
		static fmt::string::view u;
		if (empty(u))
		{
			auto const args = env::opt::arguments();
			assert(not empty(args));
			auto const path = args.front();
			assert(not empty(path));
			auto const dirs = fmt::dir::split(path);
			assert(not empty(dirs));
			auto const name = dirs.back();
			assert(not empty(name));
			auto const first = name.find_first_not_of("./");
			auto const last = name.rfind(sys::ext::image);
			u = name.substr(first, last);
		}
		return u;
	}

	fmt::string::view config()
	{
		static fmt::string s;
		if (empty(s))
		{
			auto const filename = make_ini();
			for (auto const dirs : { env::dir::config(), env::dir::paths() })
			{
				using namespace env::dir;
				if (find(dirs, regx(filename) || to(s) || stop))
				{
					break;
				}
			}
		}
		return s;
	}

	fmt::string::in::ref get(fmt::string::in::ref in)
	{
		auto writer = registry().write();
		doc::ini::ref slice = *writer;
		return in >> slice;
	}

	fmt::string::out::ref put(fmt::string::out::ref out)
	{
		auto const reader = registry().read();
		doc::ini::cref slice = *reader;
		return out << slice;
	}

	bool got(pair key)
	{
		return registry().read()->got(key);
	}

	fmt::string::view get(pair key)
	{
		return registry().read()->get(key);
	}

	bool set(pair key, fmt::string::view value)
	{
		return registry().write()->set(key, value);
	}

	bool got(word key)
	{
		return not empty(get(key));
	}

	fmt::string::view get(word key)
	{
		auto const u = fmt::str::get(key);
		// First look for argument
		auto const args = arguments();
		for (auto const a : args)
		{
			auto const e = fmt::to_pair(a);
			if (e.first == u)
			{
				return e.second;
			}
		}
		// Second look in environment
		auto value = env::var::get(u);
		if (empty(value))
		{
			// Finally look in options table
			value = env::opt::get(make_pair(key));
		}
		return value;
	}

	bool set(word key, fmt::string::view value)
	{
		return set(make_pair(key), value);
	}

	fmt::string::view::vector put(int argc, char** argv, commands cmd)
	{
		assert(nullptr == argv[argc]);
		// Push a view to command line arguments
		std::copy(argv, argv + argc, std::back_inserter(list));
		// Arguments not part of a command
		fmt::string::view::vector extra;
		// Command line range
		auto const end = cmd.end();
		auto current = end;
		// Skip the path to the program image
		for (int index = 1; index < argc; ++index)
		{
			auto const [argn, next] = find_next(argv + index, cmd);

			auto count = 0;
			if (end != current)
			{
            	// Set argument values as option
				count = std::min(argn, current->argn);
				auto const sub = fmt::string::view::span(list.data() + index, count);
				auto const value = doc::ini::join(sub);
				auto const key = fmt::str::set(current->name);
				env::opt::set(key, value);
			}

			if (end != next)
			{
				// Set value to some default value
				auto const key = fmt::str::set(next->name);
				env::opt::set(key, true);
			}

			if (1 < argn)
			{
				// Non-command arguments to be returned in single vector
				auto const sub = fmt::string::view::span(list.data() + index + 1 + count, argn - count);
				extra.insert(extra.end(), sub.begin(), sub.end());
			}

			index += argn;
			current = next;
		}
		return extra;
	}
}
