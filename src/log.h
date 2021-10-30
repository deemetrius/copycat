#pragma once
#include <source_location>
#include <fstream>
#include <iomanip>
#include <ctime>

struct t_log {
	std::wofstream out_;

	static t_log & instance() {
		static t_log ret;
		return ret;
	}

	template <class T>
	void add(const T & msg, const std::source_location pos) {
		if( out_.is_open() ) {
			const std::time_t tc = std::time(nullptr);
			out_ << msg << L"\n" << pos.file_name()
			<< L" [" << pos.line() << L":" << pos.column() << L"] "
			<< std::put_time(std::localtime(&tc), L"%Y-%m-%d #%w %H:%M:%S") << std::endl;
		}
	}
};

template <class T>
inline void log(const T & msg, const std::source_location pos = std::source_location::current() ) {
	t_log::instance().add(msg, pos);
}
