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

struct t_dlu : public t_dlu_base {
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
};

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

template <hold_text Class, WORD Id, DWORD Style, int X, int Y, int W, int H>
struct is_window {
	static inline omg init(const t_dlu_base & dlu, const RECT & rc) {
		const int x = dlu.pixel_x(X), y = dlu.pixel_y(Y);
		int w, h;
		if constexpr( W > 0 ) { w = dlu.pixel_x(W); } else { w = rc.right - x - dlu.pixel_x(- W); }
		if constexpr( H > 0 ) { h = dlu.pixel_y(H); } else { h = rc.bottom - y - dlu.pixel_y(- H); }
		return {x, y, w, h};
	}

	static HWND create(const t_dlu_base & dlu, const RECT & rc, LPCTSTR title, HWND parent, HINSTANCE h_inst) {
		omg pos = init(dlu, rc);
		return CreateWindow(
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
	}

	static BOOL resize(const t_dlu_base & dlu, const RECT & rc, HWND wnd) {
		omg pos = init(dlu, rc);
		return MoveWindow(wnd, pos.x, pos.y, pos.w, pos.h, TRUE);
	}
};

namespace fs = std::filesystem;

constexpr TCHAR
empty_str		[] = _T(""),
env_data		[] = _T("AppData"),
appname			[] = _T("copycat"),
data_dir		[] = _T("dt_copycat"),
wnd_class_name	[] = _T("copycat_main");
constexpr wchar_t
tx_edit			[] = L"\U0001F589",
tx_exit			[] = L"\U0001F5F6";

using text = ex::basic_text<TCHAR>;

namespace detail {

template <class To, class From> struct wrap;
template <class Same> struct wrap<Same, Same> {
	using result = ex::basic_text<Same>;

	template <class Traits, class Allocator>
	static result conv(const std::basic_string<Same, Traits, Allocator> & str) {
		return result(ex::n_from_string::val, str);
	}
};

template <> struct wrap<wchar_t, char> {
	using result = ex::basic_text<wchar_t>;

	template <class Traits, class Allocator>
	static result conv(const std::basic_string<char, Traits, Allocator> & str) {
		result ret;
		const char * src = str.c_str();
		if( std::size_t src_len = str.size(); int len = MultiByteToWideChar(CP_UTF8, 0, src, src_len, nullptr, 0) ) {
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

	template <class Traits, class Allocator>
	result conv(const std::basic_string<wchar_t, Traits, Allocator> & str) {
		result ret;
		const wchar_t * src = str.c_str();
		if( std::size_t src_len = str.size(); int len = WideCharToMultiByte(CP_UTF8, 0, src, src_len, NULL, 0, NULL, NULL) ) {
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

struct t_data {
	struct t_item {
		bool is_visible = true, is_value_hidden = false;
		text name, value, combined;

		void make_combined() {
			combined = is_value_hidden ? name : ex::actions_text::implode({name, value}, _T("\t") );
		}

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
	using t_string = std::basic_string<char>;
	using t_items = std::vector<t_item>;

	t_items items;
	fs::path path;

	bool read() {
		//
		{
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
		}
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
				detail::wrap<TCHAR, char>::conv(name),
				detail::wrap<TCHAR, char>::conv(value)
			).make_combined();
		}
		return true;
	}
};

struct app {
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
		id_exit = 3, id_edit, id_add, id_tray,
		id_name_label, id_name, id_value_hidden, id_value, id_list,
		id_first = 100, id_step = 10
	};
	enum n_ui : int {
		ui_left = dlu_space_margin_x,
		ui_top_name_label = dlu_space_margin_y,
		ui_top_name = ui_top_name_label + dlu_label_y + dlu_space_label_near_y,
		ui_top_value_hidden = ui_top_name + dlu_edit_y + dlu_space_related_y,
		ui_top_value = ui_top_value_hidden + dlu_check_y + dlu_space_label_near_y,
		ui_height_value = 50,
		ui_top_list = ui_top_value + ui_height_value + dlu_space_related_y
	};

	using t_name_label = is_window<WC_STATIC, id_name_label,
		WS_VISIBLE | WS_CHILD |
		SS_LEFTNOWORDWRAP | SS_NOTIFY,
		ui_left, ui_top_name_label, dlu_button_x * 2, dlu_label_y
	>;
	using t_name = is_window<WC_EDIT, id_name,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP |
		ES_LEFT,
		ui_left, ui_top_name, - dlu_space_margin_x, dlu_edit_y
	>;
	using t_value_hidden = is_window<WC_BUTTON, id_value_hidden,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP |
		BS_AUTOCHECKBOX | BS_TEXT,
		ui_left, ui_top_value_hidden, dlu_button_x * 2, dlu_check_y
	>;
	using t_value = is_window<WC_EDIT, id_value,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP |
		ES_LEFT | ES_MULTILINE | ES_WANTRETURN,
		ui_left, ui_top_value, - dlu_space_margin_x, ui_height_value
	>;
	using t_list = is_window<WC_LISTVIEW, id_list,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER |
		LVS_REPORT | LVS_EDITLABELS | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
		dlu_space_smallest_x, ui_top_list, - dlu_space_smallest_x, - dlu_space_smallest_y
	>;

	t_data data;
	t_dlu dlu;
	HWND wnd_main, wnd_name_label, wnd_name, wnd_value_hidden, wnd_value, wnd_list;
	HMENU menu_main = NULL;

	app() : dlu(n_not_init::val) {}
	~app() {
		if( menu_main ) DestroyMenu(menu_main);
	}

	void ui_create(HINSTANCE h_inst) {
		RECT rc;
		GetClientRect(wnd_main, &rc);

		wnd_name_label		= t_name_label::create(dlu, rc, _T("&Name"), wnd_main, h_inst);
		wnd_name			= t_name::create(dlu, rc, empty_str, wnd_main, h_inst);
		wnd_value_hidden	= t_value_hidden::create(dlu, rc, _T("&Hide value"), wnd_main, h_inst);
		wnd_value			= t_value::create(dlu, rc, empty_str, wnd_main, h_inst);

		/*cat.wnd_exit = CreateWindow(
			WC_BUTTON, // wnd_class
			tx_exit, // title
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // style
			dlu.space_margin_x, dlu.space_margin_y, // pos
			dlu.button_x, dlu.button_y, // size
			cat.wnd_main, // parent
			(HMENU) id_exit, // menu or id
			h_inst, // instance handler
			NULL // no creation data
		);*/

		// list
		wnd_list = t_list::create(dlu, rc, empty_str, wnd_main, h_inst);
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
	}

	bool make_menu() {
		if( menu_main ) DestroyMenu(menu_main);
		if(( menu_main = CreatePopupMenu() )) {
			AppendMenu(menu_main, MF_ENABLED | MF_STRING, id_exit, _T("&quit") );
			AppendMenu(menu_main, MF_SEPARATOR, 0, NULL);
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
					AppendMenu(menu_main, MF_ENABLED | MF_STRING, pos, it.combined->cs_);
				}
				pos += 10;
				++inf.iItem;
			}
			AppendMenu(menu_main, MF_ENABLED | MF_STRING | MF_MENUBREAK, id_edit, _T("&edit") );
			AppendMenu(menu_main, MF_SEPARATOR, 0, NULL);
			pos = id_first +1;
			for( const t_data::t_item & it : data.items ) {
				if( it ) AppendMenu(menu_main, MF_ENABLED | MF_STRING, pos, _T("&edit") );
				pos += 10;
			}
		}
		return menu_main;
	}

	static LRESULT CALLBACK proc_main(HWND, UINT, WPARAM, LPARAM);

	/*void ui_list_rect(RECT & rc) const {
		GetClientRect(wnd_main, &rc);
		rc.left += dlu.space_smallest_x;
		rc.top += dlu.pixel_y(ui_top_list);
		rc.right -= dlu.space_smallest_x;
		rc.bottom -= dlu.space_smallest_y;
	}*/

	void ui_main_restore() const {
		ShowWindow(wnd_main, SW_SHOW);
		const int restore = IsZoomed(wnd_main) ? SW_SHOWMAXIMIZED : SW_RESTORE;
		ShowWindow(wnd_main, restore);
		SetForegroundWindow(wnd_main);
	}
};
