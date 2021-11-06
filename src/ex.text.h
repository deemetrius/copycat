#pragma once
#include "ex.ref.h"

#include <cstring>
#include <cwchar>
#include <initializer_list>

namespace ex {

template <class T>
struct traits_text;

template <>
struct traits_text<wchar_t> {
	EX_TYPE_ALIAS(wchar_t)

	static constexpr type
	v_empty[] = L"",
	v_space[] = L" \f\n\r\t\v";

	inline static id length(const_pointer cs) {
		return std::wcslen(cs);
	}

	inline static id spn(const_pointer dest, const_pointer src) {
		return std::wcsspn(dest, src);
	}

	inline static const_pointer chr(const_pointer str, type ch) {
		return std::wcschr(str, ch);
	}
	inline static pointer chr(pointer str, type ch) {
		return std::wcschr(str, ch);
	}

	inline static pointer copy_n(pointer dest, const_pointer src, id count) {
		return std::wcsncpy(dest, src, count);
	}

	inline static id compare(const_pointer s1, const_pointer s2) {
		return std::wcscmp(s1, s2);
	}
};

template <>
struct traits_text<char> {
	EX_TYPE_ALIAS(char)

	static constexpr type
	v_empty[] = "",
	v_space[] = " \f\n\r\t\v";

	inline static id length(const_pointer cs) {
		return std::strlen(cs);
	}

	inline static id spn(const_pointer dest, const_pointer src) {
		return std::strspn(dest, src);
	}

	inline static const_pointer chr(const_pointer str, type ch) {
		return std::strchr(str, ch);
	}
	inline static pointer chr(pointer str, type ch) {
		return std::strchr(str, ch);
	}

	inline static pointer copy_n(pointer dest, const_pointer src, id count) {
		return std::strncpy(dest, src, count);
	}

	inline static id compare(const_pointer s1, const_pointer s2) {
		return std::strcmp(s1, s2);
	}
};

//

enum class n_from_new { val };
enum class n_empty { val };
enum class n_from_string { val };
enum class n_copy { val };

namespace detail {

template <class T>
struct keep_text : public with_ref_count {
	EX_TYPE_ALIAS(T)

	const_pointer cs_;
	pointer s_ = nullptr;
	id len_;

	keep_text(const_pointer cs, id len) : cs_(cs), len_(len) {}
	keep_text(n_from_new, pointer s, id len) : cs_(s), s_(s), len_(len) {}
	~keep_text() { delete [] s_; }
};

} // ns: detail

//

template <class T, template <class C> class Traits = traits_text>
struct basic_text : public ref< detail::keep_text<T>, with<closer_one>::closer_cnt > {
	EX_TYPE_ALIAS(T)
	using keep = detail::keep_text<T>;
	using traits = Traits<T>;
	using base = ref< detail::keep_text<T>, with<closer_one>::closer_cnt >;

	basic_text(n_empty) {
		this->h_ = new keep(traits::v_empty, 0);
	}
	static basic_text & inst_empty() {
		static basic_text tx(n_empty::val);
		return tx;
	}

	basic_text() : base( inst_empty() ) {}
	basic_text(n_from_new nn, pointer s, id len) {
		this->h_ = new keep(nn, s, len);
	}
	basic_text(n_copy, const_pointer cs, id len) : basic_text() {
		if( len ) {
			pointer s;
			base::operator = (new keep(n_from_new::val, s = new type[len +1], len) );
			traits::copy_n(s, cs, len);
			s[len] = 0;
		}
	}
	basic_text(const_pointer cs_begin, const_pointer cs_end) : basic_text(n_copy::val, cs_begin, cs_end - cs_begin) {}
	template <class S>
	basic_text(n_from_string, const S & str) : basic_text(n_copy::val, str.c_str(), str.size() ) {}

	template <id N>
	basic_text(const type (& cs)[N]) {
		this->h_ = new keep(cs, N -1);
	}
	basic_text(const type (& cs)[1]) : base( inst_empty() ) {}

	// copy
	basic_text(const basic_text & r) : base(r) {}
	basic_text & operator = (const basic_text & r) {
		base::operator = (r);
		return *this;
	}

	operator bool () const { return this->h_->len_; }
	bool operator ! () const { return !this->h_->len_; }

	id calc_len() const {
		return this->h_->len_ = traits::length(this->h_->cs_);
	}
};

using wtext = basic_text<wchar_t>;
using text = basic_text<char>;

struct actions_text {
	template <class L, class C, template <class C1> class T>
	static basic_text<C, T> detail_implode(L & items, const basic_text<C, T> & sep) {
		using result = basic_text<C, T>;
		using traits = typename result::traits;
		typename L::size_type count = items.size();
		switch( count ) {
			case 0: return result::inst_empty();
			case 1: return *items.begin();
		}
		id len = 0;
		for( const result & it : items ) {
			len += it->len_;
		}
		typename result::pointer str;
		if( sep ) {
			id sep_len = sep->len_;
			len += sep_len * (count - 1);
			result ret(n_from_new::val, str = new C[len +1], len);
			bool not_first = false;
			for( const result & it : items ) {
				if( not_first ) {
					traits::copy_n(str, sep->cs_, sep_len);
					str += sep_len;
				} else {
					not_first = true;
				}
				traits::copy_n(str, it->cs_, it->len_);
				str += it->len_;
			}
			*str = 0;
			return ret;
		} else {
			result ret(n_from_new::val, str = new C[len +1], len);
			for( const result & it : items ) {
				traits::copy_n(str, it->cs_, it->len_);
				str += it->len_;
			}
			*str = 0;
			return ret;
		}
	}

	static wtext implode(std::initializer_list<wtext> items, wtext sep = wtext::inst_empty() ) {
		return detail_implode(items, sep);
	}

	static text implode(std::initializer_list<text> items, text sep = text::inst_empty() ) {
		return detail_implode(items, sep);
	}

	template <class C, template <class C1> class T>
	static basic_text<C, T> trim(const basic_text<C, T> & tx) {
		using result = basic_text<C, T>;
		using traits = typename result::traits;
		using const_pointer = typename result::const_pointer;
		if( !tx ) return tx;
		const_pointer cs = tx->cs_;
		id skip = traits::spn(cs, traits::v_space);
		cs += skip;
		id len = tx->len_ - skip;
		if( len ) for(
			const_pointer cs_end = cs + len -1;
			cs_end >= cs && traits::chr(traits::v_space, *cs_end);
			--cs_end, --len
		);
		return result(n_copy::val, cs, len);
	}
};

} // ns: ex
