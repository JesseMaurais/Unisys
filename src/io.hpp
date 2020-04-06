#ifndef io_hpp
#define io_hpp "Standard Input/Output"

#include <cstring>
#include <streambuf>
#include "file.hpp"
#include "dig.hpp"
#include "fwd.hpp"
#include "tmp.hpp"

namespace fmt
{
	template
	<
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	struct basic_streambuf : fwd::basic_streambuf<Char, Traits>
	{
		using base = fwd::basic_streambuf<Char, Traits>;
		using char_type = typename base::char_type;
		using traits_type = typename base::traits_type;
		using size_type = std::streamsize;
		using int_type = typename base::int_type;

		base *setbuf(char_type *s, size_type n) override
		{
			size_type const m = n / 2;
			return setbuf(s, n - m, m);
		}

		base *setbuf(char_type *s, size_type n, size_type m)
		{
			auto t = s + n;
			auto u = t + m;
			base::setg(s, t, t);
			base::setp(t, u);
			return this;
		}

	protected:

		int_type overflow(int_type c) override
		{
			constexpr int_type eof = traits_type::eof();
			if (base::pptr() == base::epptr())
			{
				if (-1 == sync()) c = eof;
			}
			if (traits_type::eq_int_type(eof, c))
			{
				base::setp(nullptr, nullptr);
			}
			else
			{
				*base::pptr() = traits_type::to_char_type(c);
			}
			return traits_type::not_eof(c);
		}

		int_type underflow() override
		{
			if (base::gptr() == base::egptr())
			{
				auto const max = base::egptr() - base::eback();
				auto const n = base::sgetn(base::eback(), max);
				if (0 < n)
				{
					auto const diff =  max - n;
					auto const sz = n * sizeof (char_type);
					std::memmove(base::eback() + diff, base::eback(), fmt::to_size(sz));
					base::gbump((int) -n);
				}
				else
				{
					base::setg(nullptr, nullptr, nullptr);
					return traits_type::eof();
				}
			}
			return traits_type::to_int_type(*base::gptr());
		}

		int sync() override
		{
			if (base::pbase() != base::pptr())
			{
				auto const off = base::pptr() - base::pbase();
				auto const n = base::sputn(base::pbase(), off);
				if (0 < n)
				{
					auto const diff = off - n;
					auto const sz = diff * sizeof (char_type);
					std::memmove(base::pbase(), base::pbase() + n, fmt::to_size(sz));
					base::pbump((int) -n);
				}
				return n < 0 ? -1 : 0;
			}
			return base::pptr() != base::epptr() ? 0 : -1;
		}
	};

	using streambuf = basic_streambuf<char>;
	using wstreambuf = basic_streambuf<wchar_t>;


	template
	<
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	class basic_stringbuf : public basic_streambuf<Char, Traits, Alloc>
	{
		using base = basic_streambuf<Char, Traits>;
		using string = basic_string<Char, Traits, Alloc>;
		using char_type = typename base::char_type;
		using size_type = typename base::size_type;

		basic_stringbuf() = default;
		basic_stringbuf(size_type n)
		{
			setbufsiz(n);
		}

		auto setbufsiz(size_type n)
		{
			buf.resize(fmt::to_size(n));
			return base::setbuf(buf.data(), n);
		}

		auto setbufsiz(size_type n, size_type m)
		{
			buf.resize(fmt::to_size(n + m));
			return base::setbuf(buf.data(), n, m);
		}

	private:

		string buf;
	};

	using stringbuf = basic_stringbuf<char>;
	using wstringbuf = basic_stringbuf<wchar_t>;


	template
	<
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	class basic_buf : public basic_stringbuf<Char, Traits, Alloc>
	{
		using base = basic_stringbuf<Char, Traits, Alloc>;
		using size_type = typename base::size_type;
		using char_type = typename base::char_type;

		basic_buf(env::file::stream const& obj) : f(obj) { };

	protected:

		env::file::stream const& f;

	private:

		size_type xsputn(char_type const *s, size_type n) override
		{
			return f.write(s, fmt::to_size(n));
		}

		size_type xsgetn(char_type *s, size_type n) override
		{
			return f.read(s, fmt::to_size(n));
		}

		using base::base;
	};

	template
	<
		template <class, class> class Stream,
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	struct basic_stream : unique, Stream<Char, Traits>, basic_buf<Char, Traits, Alloc>
	{
		using stream = Stream<Char, Traits>;
		using buf = basic_buf<Char, Traits, Alloc>;

		basic_stream(env::file::stream const& f) 
		: buf(f), stream(this)
		{ }
	};

	template 
	<
		class Char, 
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	using basic_istream = basic_stream
	<
		fwd::basic_istream, Char, Traits, Alloc
	>;

	using istream = basic_istream<char>;
	using wistream = basic_istream<wchar_t>;

	template
	<
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	using basic_ostream = basic_stream
	<
		fwd::basic_ostream, Char, Traits, Alloc
	>;

	using ostream = basic_ostream<char>;
	using wostream = basic_ostream<wchar_t>;

	template
	<
		class Char,
		template <class> class Traits = std::char_traits,
		template <class> class Alloc = std::allocator
	>
	using basic_iostream = basic_stream
	<
		fwd::basic_iostream, Char, Traits, Alloc
	>;

	using iostream = basic_iostream<char>;
	using wiostream = basic_iostream<wchar_t>;
}

#endif // file
