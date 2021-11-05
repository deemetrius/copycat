#pragma once
#include <cstddef>

namespace ex {

using id = std::ptrdiff_t;
using uid = std::size_t;

//

struct actions {
	template <class T>
	static void swap(T & v1, T & v2) {
		T tmp = v1;
		v1 = v2;
		v2 = tmp;
	}
};

//

struct with_ref_count {
	mutable id refs_ = 1;

	void ref_inc() const { ++refs_; }
	bool ref_dec() const { --refs_; return refs_ < 1; }
};

//

#define EX_TYPE_ALIAS(v_type) \
using type = v_type; \
using pointer = v_type *; \
using const_pointer = const v_type *;

template <class T>
struct with_accept_swap {
	EX_TYPE_ALIAS(T)

	static void accept_init(pointer & to, pointer & from) {
		actions::swap(to, from);
	}

	static void accept(pointer & to, pointer & from) {
		actions::swap(to, from);
	}
};

template <class T>
struct closer_one : public with_accept_swap<T> {
	EX_TYPE_ALIAS(T)

	static void close(const_pointer h) { delete h; }
};

template <class T>
struct closer_many : public with_accept_swap<T> {
	EX_TYPE_ALIAS(T)

	static void close(const_pointer h) { delete [] h; }
};

//

template <template <class C> class Closer>
struct with {
	template <class T>
	struct closer_cnt {
		EX_TYPE_ALIAS(T)
		using target_closer = Closer<T>;

		static void close(const_pointer h) { if( h && h->ref_dec() ) target_closer::close(h); }

		static void accept_init(pointer & to, pointer from) {
			from->ref_inc();
			to = from;
		}

		static void accept(pointer & to, pointer from) {
			from->ref_inc();
			close(to);
			to = from;
		}
	};
};

//

template <class T, template <class C> class Closer = closer_one>
struct ref {
	EX_TYPE_ALIAS(T)
	using closer = Closer<T>;

	mutable pointer h_ = nullptr;

	// base
	ref() = default;
	~ref() { closer::close(h_); }

	// from
	ref(pointer h) : h_(h) {}
	ref & operator = (pointer h) {
		closer::close(h_);
		h_ = h;
		return *this;
	}

	// copy
	ref(const ref & r) {
		closer::accept_init(h_, r.h_);
	}
	ref & operator = (const ref & r) {
		closer::accept(h_, r.h_);
		return *this;
	}

	//
	operator bool () const { return h_; }
	pointer operator -> () const { return h_; }
};

} // ns: ex
