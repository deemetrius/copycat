#pragma once

#if !defined(UNICODE)
#define UNICODE
#endif

#if !defined(_UNICODE)
#define _UNICODE
#endif

#define ISOLATION_AWARE_ENABLED 1
#define NTDDI_VERSION 0x06000000

#include "ex.text.h"
#include "log.h"

template <class C1, template <class C> class T1, class C2, class T2>
inline std::basic_ostream<C2, T2> & operator << (
	std::basic_ostream<C2, T2> & out,
	const ex::basic_text<C1, T1> & tx
) {
	return out << tx->cs_;
}

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>

#include <string>
#include <vector>
#include <filesystem>
#include <type_traits>
#include <algorithm>

// dlu
#define M_DLU_ENUM(v_axis, v_name, v_init) dlu_ ## v_name ## _ ## v_axis = v_init
#define M_DLU_INIT(v_axis, v_name, v_init) v_name ## _ ## v_axis ( pixel_ ## v_axis (v_init) )
#define M_DLU_CALC(v_axis, v_name, v_init) ( v_name ## _ ## v_axis = pixel_ ## v_axis (v_init) )
#define M_DLU_DATA(v_axis, v_name, v_init) v_name ## _ ## v_axis
#define M_DLU_ALIAS_ENUM(v_axis, v_name, v_init) dlu_ ## v_name ## _ ## v_axis = dlu_ ## v_init ## _ ## v_axis
#define M_DLU_ALIAS_INIT(v_axis, v_name, v_init) v_name ## _ ## v_axis ( v_init ## _ ## v_axis )
#define M_DLU_ALIAS_CALC(v_axis, v_name, v_init) ( v_name ## _ ## v_axis = v_init ## _ ## v_axis )
#define M_DLU_POINT(v_kind, v_name, v_init_x, v_init_y) v_kind(x, v_name, v_init_x), v_kind(y, v_name, v_init_y)
#define M_DLU_ALIAS(v_from, v_name, v_init) v_from(x, v_name, v_init), v_from(y, v_name, v_init)
#define M_DLU_MEMBERS(v_kind, v_from) \
	v_kind(y,			label,							8				), \
	M_DLU_POINT(v_kind,	button,							50, 14			), \
	v_kind(y,			check,							10				), \
	v_from(y,			radio,							check			), \
	v_from(y,			edit,							button			), \
	v_from(y,			dropdown,						check			), \
	v_from(y,			dropdown_combo,					button			), \
	M_DLU_POINT(v_kind,	space_smallest,					2, 2			), \
	M_DLU_POINT(v_kind,	space_margin,					7, 7			), \
	M_DLU_POINT(v_kind,	space_related,					4, 4			), \
	M_DLU_ALIAS(v_from,	space_unrelated,				space_margin	), \
	M_DLU_ALIAS(v_from,	space_paragraph,				space_margin	), \
	M_DLU_POINT(v_kind,	space_label_near,				3, 3			), \
	M_DLU_POINT(v_kind,	space_group_margin_top_left,	9, 14			), \
	M_DLU_ALIAS(v_from,	space_group_margin_bot_right,	space_margin	)
// https://docs.microsoft.com/en-us/previous-versions/ms997619(v=msdn.10)?redirectedfrom=MSDN

enum class n_not_init { val };
enum n_dlu : int { dlu_cx = 4, dlu_cy = 8, M_DLU_MEMBERS(M_DLU_ENUM, M_DLU_ALIAS_ENUM) };

struct t_dlu_base {
	// types
	using both = LONG;
	using type = int;

	// actions
	type pixel_x(type tpl) const { return MulDiv(tpl, base_x, dlu_cx); }
	type pixel_y(type tpl) const { return MulDiv(tpl, base_y, dlu_cy); }

	// construct
	t_dlu_base(both units = GetDialogBaseUnits() ) : t_dlu_base( LOWORD(units), HIWORD(units) ) {}
	t_dlu_base(type bx, type by) : base_x(bx), base_y(by) {}
	t_dlu_base(n_not_init) {}

	// rescale
	void rescale(both units = GetDialogBaseUnits() ) { rescale( LOWORD(units), HIWORD(units) ); }
	void rescale(type bx, type by) { base_x = bx; base_y = by; }

	// data
	type base_x, base_y;
};

/*struct t_dlu : public t_dlu_base {
	// construct
	t_dlu(both units = GetDialogBaseUnits() ) : t_dlu( LOWORD(units), HIWORD(units) ) {}
	t_dlu(type bx, type by) : t_dlu_base(bx, by), M_DLU_MEMBERS(M_DLU_INIT, M_DLU_ALIAS_INIT) {}
	t_dlu(n_not_init) {}

	// rescale
	void rescale(both units = GetDialogBaseUnits() ) { rescale( LOWORD(units), HIWORD(units) ); }
	void rescale(type bx, type by) {
		base_x = bx; base_y = by;
		M_DLU_MEMBERS(M_DLU_CALC, M_DLU_ALIAS_CALC);
	}

	// data
	type M_DLU_MEMBERS(M_DLU_DATA, M_DLU_DATA);
};*/

template <class T, ex::uid N>
struct hold_text {
	T s_[N];

	using type = T;
	enum { len = N -1 };

	constexpr hold_text(const T (& s)[N]) { std::copy_n(s, N, s_); }
	//constexpr ex::uid size() const { return N -1; }
};

struct omg {
	int x, y, w, h;
};

template <class T>
struct is_window_host {
	using h_member = HWND T::*;

	template <h_member Member>
	struct base_is_window {
		static omg & pos_instance() {
			static omg ret;
			return ret;
		}
	};

	template <h_member Member, hold_text Class, WORD Id, DWORD Style, int X, int Y, int W, int H>
	struct is_window : public base_is_window<Member> {
		using base = base_is_window<Member>;

		static void calc_position(const t_dlu_base & dlu) {
			omg & pos = base::pos_instance();
			if constexpr( X < 0 ) { pos.x = dlu.pixel_x(- X); } else { pos.x = dlu.pixel_x(X); }
			if constexpr( Y < 0 ) { pos.y = dlu.pixel_y(- Y); } else { pos.y = dlu.pixel_y(Y); }
			if constexpr( W < 0 ) { pos.w = dlu.pixel_x(- W); } else { pos.w = dlu.pixel_x(W); }
			if constexpr( H < 0 ) { pos.h = dlu.pixel_y(- H); } else { pos.h = dlu.pixel_y(H); }
		}

		static inline omg init(const RECT & rc) {
			omg pos = base::pos_instance();
			if constexpr( X < 0 ) { pos.x = rc.right - pos.x; }
			if constexpr( Y < 0 ) { pos.y = rc.bottom - pos.y; }
			if constexpr( W < 0 ) { pos.w = rc.right - pos.x - pos.w; }
			if constexpr( H < 0 ) { pos.h = rc.bottom - pos.y - pos.h; }
			return pos;
		}

		static HWND create(T * obj, const RECT & rc, LPCTSTR title, HWND parent, HINSTANCE h_inst, HFONT font = NULL) {
			calc_position(obj->dlu);
			omg pos = init(rc);
			HWND ret = obj->*Member = CreateWindow(
				Class.s_, // wnd_class
				title, // title
				Style, // style
				pos.x, pos.y, // pos
				pos.w, pos.h, // size
				parent, // parent
				(HMENU) Id, // menu or id
				h_inst, // instance handler
				NULL // no creation data
			);
			if( ret && font ) SendMessage(ret, WM_SETFONT, (WPARAM) font, FALSE);
			return ret;
		}

		static BOOL resize(T * obj, const RECT & rc) {
			omg pos = init(rc);
			return MoveWindow(obj->*Member, pos.x, pos.y, pos.w, pos.h, TRUE);
		}
	};

};

namespace fs = std::filesystem;

constexpr TCHAR
empty_str		[] = _T(""),
env_data		[] = _T("AppData"),
appname			[] = _T("copycat"),
data_dir		[] = _T("dt_copycat"),
wnd_class_name	[] = _T("copycat_main"),
wnd_class_aux	[] = _T("copycat_aux");
constexpr wchar_t
tx_edit			[] = L"\U0001F589",
tx_clear		[] = L"\U0001F5F6";

using text = ex::basic_text<TCHAR>;

namespace detail {

template <class To, class From> struct wrap;

/*template <class Same> struct wrap<Same, Same> {
	using result = ex::basic_text<Same>;

	static result conv(const Same * src, ex::id src_len) {
		return result::inst_empty();
	}
};*/

template <> struct wrap<wchar_t, char> {
	using result = ex::basic_text<wchar_t>;

	static result conv(const char * src, ex::id src_len) {
		result ret;
		if( int len = MultiByteToWideChar(CP_UTF8, 0, src, src_len, nullptr, 0) ) {
			wchar_t * s;
			ret = result(ex::n_from_new::val, s = new wchar_t[len +1], len);
			len = MultiByteToWideChar(CP_UTF8, 0, src, src_len, s, len);
			s[len] = 0;
			ret.calc_len();
		}
		return ret;
	}
};

template <> struct wrap<char, wchar_t> {
	using result = ex::basic_text<char>;

	static result conv(const wchar_t * src, ex::id src_len) {
		result ret;
		if( int len = WideCharToMultiByte(CP_UTF8, 0, src, src_len, NULL, 0, NULL, NULL) ) {
			char * s;
			ret = result(ex::n_from_new::val, s = new char[len +1], len);
			len = WideCharToMultiByte(CP_UTF8, 0, src, src_len, s, len, NULL, NULL);
			s[len] = 0;
			ret.calc_len();
		}
		return ret;
	}
};

} // ns: detail

template <class To>
struct wrap {
	using result = ex::basic_text<To>;

	template <class From, class Traits, class Allocator>
	static result conv(const std::basic_string<From, Traits, Allocator> & str) {
		if constexpr( std::is_same_v<To, From> ) {
			return result(ex::n_from_string::val, str);
		} else {
			return detail::wrap<To, From>::conv(str.c_str(), str.size() );
		}
	}

	template <class From>
	static result conv(const ex::basic_text<From> & tx) {
		if constexpr( std::is_same_v<To, From> ) {
			return tx;
		} else {
			return detail::wrap<To, From>::conv(tx->cs_, tx->len_);
		}
	}
};

struct t_data {
	struct t_item {
		bool is_visible = true, is_value_hidden = false;
		text name, value, combined;

		void make_combined() {
			combined = is_value_hidden ? name : ex::actions_text::implode({name, value}, _T("\t") );
		}

		bool is_valid() const { return static_cast<bool>(name) && static_cast<bool>(value); }

		operator bool () const { return is_visible; }

		bool put(HWND wnd) const {
			bool ret = false;
			if( !OpenClipboard(wnd) ) {
				log(L"error: OpenClipboard");
				return false;
			}
			if( !EmptyClipboard() ) {
				log(L"error: EmptyClipboard");
			}
			if( HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (value->len_ +1) * sizeof(TCHAR) ); !mem ) {
				log(L"error: GlobalAlloc");
			} else {
				LPTSTR lock = (LPTSTR) GlobalLock(mem);
				text::traits::copy_n(lock, value->cs_, value->len_);
				lock[value->len_] = 0;
				GlobalUnlock(mem);
				if( !SetClipboardData(std::is_same_v<TCHAR, char> ? CF_TEXT : CF_UNICODETEXT, mem) ) {
					log(L"error: SetClipboardData");
				} else {
					ret = true;
				}
			}
			if( !CloseClipboard() ) {
				log(L"error: CloseClipboard");
				return false;
			}
			return ret;
		}
	};

	using t_in = std::basic_ifstream<char>;
	using t_out = std::basic_ofstream<char>;
	using t_string = std::basic_string<char>;
	using t_text = ex::basic_text<char>;
	using t_items = std::vector<t_item>;

	t_items items;
	fs::path path;

	bool check() const {
		std::error_code ec;
		fs::file_status status = fs::status(path, ec);
		if( ec ) {
			log(L"error: Checking config fail");
			return false;
		}
		if( status.type() == fs::file_type::not_found ) {
			log("log: Config is absent");
			return false;
		}
		if( status.type() != fs::file_type::regular ) {
			log(L"error: Config should be regular file");
			return false;
		}
		return true;
	}

	bool read() {
		if( !check() ) return false;
		t_in in;
		in.open(path, std::ios_base::binary);
		if( !in.is_open() ) {
			log(L"error: Unable to open config for reading");
			return false;
		}
		t_string name, value, flags;
		items.clear();
		while( std::getline(in, name) && std::getline(in, value) && std::getline(in, flags) ) {
			items.emplace_back(
				flags.size() >= 1 && flags[0] == '1',
				flags.size() >= 2 && flags[1] == '1',
				wrap<TCHAR>::conv(name),
				ex::actions_text::escape_back( wrap<TCHAR>::conv(value) )
			).make_combined();
		}
		return true;
	}

	bool write() const {
		if( !check() ) return false;
		t_out out;
		out.open(path, std::ios_base::binary);
		if( !out.is_open() ) {
			log(L"error: Unable to open config for reading");
			return false;
		}
		t_text name, value;
		for( const t_item & it : items ) {
			name = wrap<char>::conv(it.name);
			value = ex::actions_text::escape( wrap<char>::conv(it.value) );
			out << name << '\n' << value << '\n' <<
			(it.is_visible ? '1' : '0') << (it.is_value_hidden ? '1' : '0') << '\n';
		}
		return true;
	}
};

struct base_app {
	t_dlu_base dlu;
	HWND wnd_main, wnd_name_label, wnd_name, wnd_clear, wnd_value_hidden, wnd_value, wnd_set, wnd_add, wnd_del, wnd_list;

	base_app() : dlu(n_not_init::val) {}
};

struct app : public base_app, public is_window_host<base_app> {
	static app & instance() { static app ret; return ret; }
	static fs::path get_data_path() {
		fs::path ret;
		if( DWORD len = GetEnvironmentVariable(env_data, NULL, 0) ) {
			TCHAR * buf;
			text tx(ex::n_from_new::val, buf = new TCHAR[len], len -1);
			if( GetEnvironmentVariable(env_data, buf, len) ) {
				ret = buf;
				ret /= data_dir;
			}
		}
		return ret;
	}

	enum n_id : WORD {
		id_exit = 10, id_edit, id_tray, id_about, id_save, id_load,
		id_name_label, id_name, id_clear, id_value_hidden, id_value, id_add, id_del, id_set, id_list,
		id_first = 100, id_step = 10
	};
	enum n_ui : int {
		ui_left = dlu_space_margin_x,
		ui_width = dlu_button_x,
		ui_width_2 = dlu_edit_y,
		ui_right = ui_left + ui_width_2,
		ui_right_2 = ui_right + dlu_space_related_x,
		ui_left_2 = ui_left + ui_width + dlu_space_related_x,
		//ui_left_3 = (ui_left_2 - dlu_button_x) / 2,
		ui_top_name = dlu_space_margin_y, // + dlu_label_y + dlu_space_label_near_y,
		ui_top_name_label = ui_top_name + (dlu_edit_y - dlu_label_y) / 2,
		ui_top_value_hidden = ui_top_name + dlu_edit_y + dlu_space_related_y,
		ui_top_value = ui_top_value_hidden, // + dlu_check_y + dlu_space_label_near_y,
		ui_height_value = 70,
		ui_bottom_value = ui_top_value + ui_height_value,
		ui_top_del = ui_bottom_value - dlu_button_y,
		ui_top_add = ui_top_del - dlu_space_related_y - dlu_button_y,
		ui_top_set = ui_top_add - dlu_space_related_y - dlu_button_y,
		ui_top_list = ui_bottom_value + dlu_space_related_y
	};

	using t_name_label = is_window<&base_app::wnd_name_label, WC_STATIC, id_name_label,
		WS_VISIBLE | WS_CHILD |
		SS_NOTIFY, // SS_LEFTNOWORDWRAP SS_RIGHT
		ui_left, ui_top_name_label, ui_width, dlu_label_y
	>;
	using t_name = is_window<&base_app::wnd_name, WC_EDIT, id_name,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP |
		ES_LEFT, // ES_NOHIDESEL ES_CENTER
		ui_left_2, ui_top_name, - ui_right_2, dlu_edit_y
	>;
	using t_clear = is_window<&base_app::wnd_clear, WC_BUTTON, id_clear,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD |
		BS_PUSHBUTTON,
		- ui_right, ui_top_name, ui_width_2, dlu_edit_y
	>;
	using t_value_hidden = is_window<&base_app::wnd_value_hidden, WC_BUTTON, id_value_hidden,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP |
		BS_AUTOCHECKBOX | BS_TEXT | BS_RIGHTBUTTON | BS_TOP, // BS_PUSHLIKE BS_RIGHT
		ui_left, ui_top_value_hidden, ui_width, dlu_button_y
	>;
	using t_value = is_window<&base_app::wnd_value, WC_EDIT, id_value,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP |
		ES_LEFT | ES_MULTILINE | ES_WANTRETURN,
		ui_left_2, ui_top_value, - ui_left, ui_height_value
	>;
	using t_set = is_window<&base_app::wnd_set, WC_BUTTON, id_set,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD |
		BS_PUSHBUTTON,
		ui_left, ui_top_set, ui_width, dlu_button_y
	>;
	using t_add = is_window<&base_app::wnd_add, WC_BUTTON, id_add,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD |
		BS_PUSHBUTTON,
		ui_left, ui_top_add, ui_width, dlu_button_y
	>;
	using t_del = is_window<&base_app::wnd_del, WC_BUTTON, id_del,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD |
		BS_PUSHBUTTON,
		ui_left, ui_top_del, ui_width, dlu_button_y
	>;
	using t_list = is_window<&base_app::wnd_list, WC_LISTVIEW, id_list,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER |
		LVS_REPORT | LVS_EDITLABELS | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
		dlu_space_smallest_x, ui_top_list, - dlu_space_smallest_x, - dlu_space_smallest_y
	>;

	t_data data;
	HWND wnd_main, wnd_aux;
	HMENU menu_main, menu_tray = NULL;
	bool is_list_ready = false, is_data_changed = false;
	int current = -1;

	static LRESULT CALLBACK proc_main(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK proc_aux(HWND, UINT, WPARAM, LPARAM);

	~app() { if( menu_tray ) DestroyMenu(menu_tray); }

	void on_create(HINSTANCE h_inst) {
		RECT rc;
		GetClientRect(wnd_main, &rc);
		t_list::create(this, rc, empty_str, wnd_main, h_inst);
		ListView_SetExtendedListViewStyle(wnd_list,
			//LVS_EX_AUTOCHECKSELECT |
			LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER //| LVS_EX_AUTOSIZECOLUMNS
		);
		//
		{
			LVCOLUMN col{};
			col.mask = LVCF_WIDTH | LVCF_TEXT;
			col.cx = 170;
			col.pszText = _T("Name");
			ListView_InsertColumn(wnd_list, 0, &col);
			col.pszText = _T("Value");
			ListView_InsertColumn(wnd_list, 1, &col);
		}
		HFONT font = (HFONT) SendMessage(wnd_list, WM_GETFONT, 0, 0);
		//
		{
			HDC hdc = GetDC(wnd_list);
			SelectObject(hdc, font);
			TEXTMETRIC m;
			GetTextMetrics(hdc, &m);
			ReleaseDC(wnd_list, hdc);
			dlu.rescale(m.tmAveCharWidth, m.tmHeight);
		}
		t_list::calc_position(dlu);
		t_list::resize(this, rc);
		t_name_label::create(this, rc, _T("&Name"), wnd_main, h_inst, font);
		t_name::create(this, rc, empty_str, wnd_main, h_inst, font);
		t_clear::create(this, rc, _T("&X"), wnd_main, h_inst, font);
		t_value_hidden::create(this, rc, _T("&Hide value"), wnd_main, h_inst, font);
		t_value::create(this, rc, empty_str, wnd_main, h_inst, font);
		t_set::create(this, rc, _T("&Set"), wnd_main, h_inst, font);
		t_add::create(this, rc, _T("&Add"), wnd_main, h_inst, font);
		t_del::create(this, rc, _T("&Del"), wnd_main, h_inst, font);
	}

	bool make_menu_only() {
		if( !is_data_changed ) return true;
		if( menu_tray ) DestroyMenu(menu_tray);
		if(( menu_tray = CreatePopupMenu() )) {
			AppendMenu(menu_tray, MF_ENABLED | MF_STRING, id_exit, _T("&quit") );
			AppendMenu(menu_tray, MF_SEPARATOR, 0, NULL);
			UINT_PTR pos = id_first;
			for( const t_data::t_item & it : data.items ) {
				if( it ) {
					AppendMenu(menu_tray, MF_ENABLED | MF_STRING, pos, it.combined->cs_);
				}
				pos += 10;
			}
			AppendMenu(menu_tray, MF_ENABLED | MF_STRING | MF_MENUBREAK, id_edit, _T("&edit") );
			AppendMenu(menu_tray, MF_SEPARATOR, 0, NULL);
			pos = id_first +1;
			for( const t_data::t_item & it : data.items ) {
				if( it ) AppendMenu(menu_tray, MF_ENABLED | MF_STRING, pos, _T("&edit") );
				pos += 10;
			}
			is_data_changed = false;
		}
		return menu_tray;
	}

	bool make_menu() {
		if( menu_tray ) DestroyMenu(menu_tray);
		if(( menu_tray = CreatePopupMenu() )) {
			AppendMenu(menu_tray, MF_ENABLED | MF_STRING, id_exit, _T("&quit") );
			AppendMenu(menu_tray, MF_SEPARATOR, 0, NULL);
			TCHAR none[] = _T("");
			LVITEM inf{};
			inf.mask = LVIF_DI_SETITEM | LVIF_TEXT; // LVIF_STATE
			inf.iItem = 0;
			UINT_PTR pos = id_first;
			for( const t_data::t_item & it : data.items ) {
				inf.iSubItem = 0;
				inf.pszText = it.name->s_ ? it.name->s_ : none;
				ListView_InsertItem(wnd_list, &inf);
				if( !it.is_value_hidden ) {
					inf.iSubItem = 1;
					inf.pszText = it.value->s_ ? it.value->s_ : none;
					ListView_SetItem(wnd_list, &inf);
				}
				if( it ) {
					ListView_SetCheckState(wnd_list, inf.iItem, TRUE);
					AppendMenu(menu_tray, MF_ENABLED | MF_STRING, pos, it.combined->cs_);
				}
				pos += 10;
				++inf.iItem;
			}
			AppendMenu(menu_tray, MF_ENABLED | MF_STRING | MF_MENUBREAK, id_edit, _T("&edit") );
			AppendMenu(menu_tray, MF_SEPARATOR, 0, NULL);
			pos = id_first +1;
			for( const t_data::t_item & it : data.items ) {
				if( it ) AppendMenu(menu_tray, MF_ENABLED | MF_STRING, pos, _T("&edit") );
				pos += 10;
			}
		}
		return menu_tray;
	}

	void on_show_tray_menu() {
		if( POINT pt; GetCursorPos(&pt) ) {
			make_menu_only();
			ShowWindow(wnd_aux, SW_SHOWNORMAL);
			SetForegroundWindow(wnd_aux);
			TrackPopupMenuEx(
				menu_tray,
				TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
				pt.x, pt.y,
				wnd_aux,
				NULL
			);
			ShowWindow(wnd_aux, SW_HIDE);
		}
	}

	void on_main_restore() const {
		ShowWindow(wnd_main, SW_SHOW);
		const int restore = IsZoomed(wnd_main) ? SW_SHOWMAXIMIZED : SW_RESTORE;
		ShowWindow(wnd_main, restore);
		SetForegroundWindow(wnd_main);
	}

	void on_list_get(int n) {
		if( !is_list_ready ) return;
		current = n;
		t_data::t_item & item = data.items[n];
		SetWindowText(wnd_name, item.name->cs_);
		SetWindowText(wnd_value, item.value->cs_);
		Button_SetCheck(wnd_value_hidden, item.is_value_hidden ? BST_CHECKED : BST_UNCHECKED);
	}

	static text get_text(HWND wnd) {
		if( ex::id len = GetWindowTextLength(wnd) ) {
			TCHAR * s;
			text ret(ex::n_from_new::val, s = new TCHAR[len +1], len);
			if( GetWindowText(wnd, s, len +1) ) {
				ret.calc_len();
				return ret;
			}
		}
		return text::inst_empty();
	}

	void on_list_check(int pos) {
		if( !is_list_ready ) return;
		t_data::t_item & item = data.items[pos];
		item.is_visible = ListView_GetCheckState(wnd_list, pos);
		is_data_changed = true;
	}

	void on_list_sel(int pos) {
		LVITEMA inf{};
		inf.mask = LVIF_STATE;
		inf.stateMask = inf.state = LVIS_SELECTED;
		inf.iItem = pos;
		ListView_SetItem(wnd_list, &inf);
	}

	void on_list_del() {
		if( !data.items.size() || current < 0 ) return;
		is_list_ready = false;
		data.items.erase(data.items.begin() + current);
		ListView_DeleteItem(wnd_list, current);
		current = -1;
		is_list_ready = true;
		is_data_changed = true;
	}

	void on_list_add() {
		int pos = data.items.size();
		t_data::t_item item;
		item.name = ex::actions_text::trim( get_text(wnd_name) );
		item.value = ex::actions_text::trim( get_text(wnd_value) );
		SetWindowText(wnd_name, item.name->cs_);
		SetWindowText(wnd_value, item.value->cs_);
		if( item.is_valid() ) {
			is_list_ready = false;
			item.is_value_hidden = (Button_GetCheck(wnd_value_hidden) == BST_CHECKED);
			item.make_combined();
			data.items.push_back(item);
			//
			TCHAR none[] = _T("");
			LVITEM inf{};
			inf.mask = LVIF_DI_SETITEM | LVIF_TEXT; // LVIF_STATE
			inf.iItem = pos;
			inf.iSubItem = 0;
			inf.pszText = item.name->s_ ? item.name->s_ : none;
			ListView_InsertItem(wnd_list, &inf);
			//
			inf.iSubItem = 1;
			inf.pszText = (!item.is_value_hidden && item.value->s_) ? item.value->s_ : none;
			ListView_SetItem(wnd_list, &inf);
			//
			ListView_SetCheckState(wnd_list, inf.iItem, TRUE);
			on_list_sel(pos);
			current = pos;
			is_list_ready = true;
			is_data_changed = true;
		}
	}

	void on_list_set() {
		if( current < 0 ) return;
		t_data::t_item item;
		item.name = ex::actions_text::trim( get_text(wnd_name) );
		item.value = ex::actions_text::trim( get_text(wnd_value) );
		SetWindowText(wnd_name, item.name->cs_);
		SetWindowText(wnd_value, item.value->cs_);
		if( item.is_valid() ) {
			is_list_ready = false;
			t_data::t_item & cur_item = data.items[current];
			item.is_visible = cur_item.is_visible;
			item.is_value_hidden = (Button_GetCheck(wnd_value_hidden) == BST_CHECKED);
			item.make_combined();
			cur_item = item;
			//
			TCHAR none[] = _T("");
			/*LVITEM inf{};
			inf.mask = LVIF_DI_SETITEM | LVIF_TEXT; // LVIF_STATE
			inf.iItem = current;
			inf.iSubItem = 0;
			inf.pszText = item.name->s_ ? item.name->s_ : none;
			ListView_SetItem(wnd_list, &inf);
			//
			inf.iSubItem = 1;
			inf.pszText = (!item.is_value_hidden && item.value->s_) ? item.value->s_ : none;
			ListView_SetItem(wnd_list, &inf);*/
			ListView_SetItemText(wnd_list, current, 0, item.name->s_ ? item.name->s_ : none);
			ListView_SetItemText(wnd_list, current, 1, item.is_value_hidden || !item.value->s_ ? none : item.value->s_);
			//
			is_list_ready = true;
			is_data_changed = true;
		}
	}
};
