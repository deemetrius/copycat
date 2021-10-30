// CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST "copycat.exe.manifest"
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

// dlu
#define DLU_ENUM(v_axis, v_name, v_init) dlu_ ## v_name ## _ ## v_axis = v_init
#define DLU_INIT(v_axis, v_name, v_init) v_name ## _ ## v_axis ( pixel_ ## v_axis (v_init) )
#define DLU_CALC(v_axis, v_name, v_init) ( v_name ## _ ## v_axis = pixel_ ## v_axis (v_init) )
#define DLU_DATA(v_axis, v_name, v_init) v_name ## _ ## v_axis
#define DLU_ALIAS_ENUM(v_axis, v_name, v_init) dlu_ ## v_name ## _ ## v_axis = dlu_ ## v_init ## _ ## v_axis
#define DLU_ALIAS_INIT(v_axis, v_name, v_init) v_name ## _ ## v_axis ( v_init ## _ ## v_axis )
#define DLU_ALIAS_CALC(v_axis, v_name, v_init) ( v_name ## _ ## v_axis = v_init ## _ ## v_axis )
#define DLU_POINT(v_kind, v_name, v_init_x, v_init_y) v_kind(x, v_name, v_init_x), v_kind(y, v_name, v_init_y)
#define DLU_ALIAS(v_from, v_name, v_init) v_from(x, v_name, v_init), v_from(y, v_name, v_init)
#define DLU_MEMBERS(v_kind, v_from) \
	v_kind(y,			label,							8				), \
	DLU_POINT(v_kind,	button,							50, 14			), \
	v_kind(y,			check,							10				), \
	v_from(y,			radio,							check			), \
	v_from(y,			edit,							button			), \
	v_from(y,			dropdown,						check			), \
	v_from(y,			dropdown_combo,					button			), \
	DLU_POINT(v_kind,	space_smallest,					2, 2			), \
	DLU_POINT(v_kind,	space_margin,					7, 7			), \
	DLU_POINT(v_kind,	space_related,					4, 4			), \
	DLU_ALIAS(v_from,	space_unrelated,				space_margin	), \
	DLU_ALIAS(v_from,	space_paragraph,				space_margin	), \
	DLU_POINT(v_kind,	space_label_near,				3, 3			), \
	DLU_POINT(v_kind,	space_group_margin_top_left,	9, 14			), \
	DLU_ALIAS(v_from,	space_group_margin_bot_right,	space_margin	)
// https://docs.microsoft.com/en-us/previous-versions/ms997619(v=msdn.10)?redirectedfrom=MSDN

enum class n_not_init { val };

struct t_dlu {
	// types
	using both = LONG;
	using type = int;
	enum n_dlu : type { dlu_cx = 4, dlu_cy = 8, DLU_MEMBERS(DLU_ENUM, DLU_ALIAS_ENUM) };

	// actions
	type pixel_x(type tpl) const { return MulDiv(tpl, base_x, dlu_cx); }
	type pixel_y(type tpl) const { return MulDiv(tpl, base_y, dlu_cy); }

	// construct
	t_dlu(both units = GetDialogBaseUnits() ) : t_dlu( LOWORD(units), HIWORD(units) ) {}
	t_dlu(type bx, type by) : base_x(bx), base_y(by), DLU_MEMBERS(DLU_INIT, DLU_ALIAS_INIT) {}
	t_dlu(n_not_init) {}

	// rescale
	void rescale(both units = GetDialogBaseUnits() ) { rescale( LOWORD(units), HIWORD(units) ); }
	void rescale(type bx, type by) {
		base_x = bx; base_y = by;
		DLU_MEMBERS(DLU_CALC, DLU_ALIAS_CALC);
	}

	// data
	type base_x, base_y, DLU_MEMBERS(DLU_DATA, DLU_DATA);
};

namespace fs = std::filesystem;

constexpr TCHAR
env_data		[] = _T("AppData"),
appname			[] = _T("copycat"),
data_dir		[] = _T("dt_copycat"),
wnd_class_name	[] = _T("copycat_main");
constexpr wchar_t
tx_edit			[] = L"\U0001F589",
tx_exit			[] = L"\U0001F5F6";

using text = ex::basic_text<TCHAR>;

enum n_id : WORD {
	id_exit = 1, id_edit, id_add, id_list, id_tray,
	id_first = 100, id_step = 10
};

enum n_ui : int { ui_list_top = 150 /*size_list_x = 200, size_list_y = 280*/ };

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

	t_data data;
	t_dlu dlu;
	HWND wnd_main, wnd_exit, wnd_list;
	HMENU menu_main = NULL;

	app() : dlu(n_not_init::val) {}
	~app() {
		if( menu_main ) DestroyMenu(menu_main);
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

	void ui_list_rect(RECT & rc) const {
		GetClientRect(wnd_main, &rc);
		rc.left += dlu.space_smallest_x;
		rc.top += ui_list_top;
		rc.right -= dlu.space_smallest_x;
		rc.bottom -= dlu.space_smallest_y;
	}

	void ui_main_restore() const {
		ShowWindow(wnd_main, SW_SHOW);
		const int restore = IsZoomed(wnd_main) ? SW_SHOWMAXIMIZED : SW_RESTORE;
		ShowWindow(wnd_main, restore);
		SetForegroundWindow(wnd_main);
	}
};

INT WINAPI WinMain(HINSTANCE h_inst, HINSTANCE h_inst_prev, PSTR args, INT cmd_show) {
	//std::setlocale(LC_ALL, "Rus");

	// %AppData%
	fs::path f_path = app::get_data_path();
	if( f_path.empty() ) return 0;

	// subdir in %AppData%
	{
		std::wstring f_path_s = f_path.wstring();
		CreateDirectoryW(f_path_s.c_str(), NULL);
	}

	// only one instance
	f_path /= appname;
	f_path += _T(".lock");
	HANDLE f_once;
	// try to lock file
	{
		std::wstring f_path_s = f_path.wstring();
		f_once = CreateFileW(
			f_path_s.c_str(), GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL
		);
	}
	if( f_once == INVALID_HANDLE_VALUE ) return 0;

	// log
	f_path.replace_extension(L"log");
	t_log::instance().out_.open(f_path, std::ios::binary | std::ios::trunc);
	log(L"build: " __DATE__ " ~ " __TIME__);

	// init common controls
	{
		INITCOMMONCONTROLSEX common{};
		common.dwSize = sizeof(INITCOMMONCONTROLSEX);
		common.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
		if( InitCommonControlsEx(&common) == FALSE ) {
			log(L"error: InitCommonControlsEx");
			return 0;
		}
	}

	// app
	app & cat = app::instance();
	f_path.replace_extension(L"txt");
	cat.data.path = f_path;
	cat.data.read();

	// main window class
	{
		WNDCLASSEX wincl{};
		wincl.cbSize = sizeof(WNDCLASSEX);
		wincl.hInstance = h_inst;
		wincl.lpszClassName = wnd_class_name;
		wincl.lpfnWndProc = app::proc_main;
		wincl.style = CS_DBLCLKS; // CS_NOCLOSE
		wincl.hIcon = LoadIcon(h_inst, MAKEINTRESOURCE(0) );
		wincl.hIconSm = NULL;
		wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
		wincl.lpszMenuName = NULL;
		wincl.cbClsExtra = 0;
		wincl.cbWndExtra = 0;
		wincl.hbrBackground = (HBRUSH) COLOR_WINDOW;
		if( !RegisterClassEx(&wincl) ) {
			log(L"error: RegisterClassEx");
			return 0;
		}
	}

	t_dlu & dlu = cat.dlu;
	dlu.rescale();
	// main window
	cat.wnd_main = CreateWindowEx(
		WS_EX_CONTROLPARENT | WS_EX_APPWINDOW, // ex_style // WS_EX_TOOLWINDOW
		wnd_class_name, // wnd_class
		_T("copycat"), // title
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SIZEBOX, // style // WS_BORDER
		CW_USEDEFAULT, // pos_x
		CW_USEDEFAULT, // pos_y
		400, // width
		300, //dlu.space_margin_y * 2 + size_list_y + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME /*SM_CYFIXEDFRAME*/) * 2, // height
		HWND_DESKTOP, // parent
		NULL, // menu
		h_inst, // instance handler
		NULL // no creation data
	);
	if( !cat.wnd_main ) {
		log(L"error: CreateWindowEx main");
		return 0;
	}

	cat.wnd_exit = CreateWindow(
		WC_BUTTON, // wnd_class
		tx_exit, // title
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // style
		dlu.space_margin_x, dlu.space_margin_y, // pos
		dlu.button_x, dlu.button_y, // size
		cat.wnd_main, // parent
		(HMENU) id_exit, // menu or id
		h_inst, // instance handler
		NULL // no creation data
	);

	// list
	{
		RECT rc;
		cat.ui_list_rect(rc);
		cat.wnd_list = CreateWindow(
			WC_LISTVIEW, // wnd_class
			_T(""), // title
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER |
			LVS_REPORT | LVS_EDITLABELS | LVS_NOSORTHEADER |
			LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_EX_AUTOSIZECOLUMNS, // style // LVS_SINGLESEL
			rc.left, rc.top, // pos
			rc.right - rc.left, rc.bottom - rc.top, // size
			cat.wnd_main, // parent
			(HMENU) id_list, // menu or id
			h_inst, // instance handler
			NULL // no creation data
		);
	}
	ListView_SetExtendedListViewStyle(cat.wnd_list,
		/*LVS_EX_AUTOCHECKSELECT |*/ LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER
	);
	//
	{
		LVCOLUMN col{};
		col.mask = LVCF_WIDTH | LVCF_TEXT;
		col.cx = 150;
		col.pszText = _T("Name");
		ListView_InsertColumn(cat.wnd_list, 0, &col);
		col.pszText = _T("Value");
		ListView_InsertColumn(cat.wnd_list, 1, &col);
	}

	//
	cat.make_menu();
	ShowWindow(cat.wnd_main, SW_SHOWNORMAL);
	SetFocus(cat.wnd_list);
	ShowWindow(cat.wnd_main, SW_HIDE);

	// tray icon
	NOTIFYICONDATA nid{};
	nid.cbSize = sizeof(nid);
	nid.uID = id_tray;
	nid.hWnd = cat.wnd_main;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_USER;
	//LoadIconMetric(NULL, IDI_APPLICATION, LIM_SMALL, &(nid.hIcon) );
	nid.hIcon = LoadIcon(h_inst, MAKEINTRESOURCE(0) );
	StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), _T("copycat") );
	Shell_NotifyIcon(NIM_ADD, &nid);

	// msg loop
	MSG messages;
	while( GetMessage(&messages, NULL, 0, 0) ) {
		if( !IsDialogMessage(cat.wnd_main, &messages) ) {
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		}
	}

	// tray icon done
	Shell_NotifyIcon(NIM_DELETE, &nid);

	// return-value is 0 - that PostQuitMessage() gave
	return messages.wParam;
}

LRESULT CALLBACK app::proc_main(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
	switch( msg ) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		{
			app & cat = app::instance();
			ShowWindow(cat.wnd_main, SW_HIDE);
		}
		return 0;
	case WM_COMMAND:
		switch( WORD code = HIWORD(w_param) ) {
		case 0:
		case 1:
		//case BN_CLICKED:
			switch( WORD id_c = LOWORD(w_param) ) {
			case id_exit:
				PostQuitMessage(0);
				return 0;
			case id_edit:
				{
					app & cat = app::instance();
					cat.ui_main_restore();
				}
				return 0;
			default:
				if( id_c >= id_first ) {
					app & cat = app::instance();
					INT id_go = id_c - id_first, id_mod = id_go % id_step;
					id_go -= id_mod;
					id_go /= id_step;
					if( static_cast<t_data::t_items::size_type>(id_go) < cat.data.items.size() ) {
						switch( id_mod ) {
						case 0:
							cat.data.items[id_go].put(cat.wnd_main);
							return 0;
						case 1:
							{
								cat.ui_main_restore();
								LVITEMA inf{};
								inf.mask = LVIF_STATE;
								inf.stateMask = inf.state = LVIS_SELECTED;
								inf.iItem = id_go;
								ListView_SetItem(cat.wnd_list, &inf);
								SetFocus(cat.wnd_list);
								return 0;
							}
						}
					}
				}
			}
			break;
		}
		break;
	case WM_SIZE:
		{
			app & cat = app::instance();
			RECT rc;
			cat.ui_list_rect(rc);
			MoveWindow(cat.wnd_list, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		return 0;
	case WM_USER:
		switch( w_param ) {
		case id_tray:
			switch( l_param ) {
			case WM_LBUTTONUP:
				{
					POINT pt;
					if( GetCursorPos(&pt) ) {
						app & cat = app::instance();
						BOOL vis = IsWindowVisible(cat.wnd_main);
						if( !vis ) ShowWindow(cat.wnd_main, SW_SHOWMINIMIZED);
						SetForegroundWindow(cat.wnd_main);
						TrackPopupMenuEx(
							cat.menu_main,
							TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
							pt.x, pt.y,
							cat.wnd_main,
							NULL
						);
						if( !vis ) ShowWindow(cat.wnd_main, SW_HIDE);
						return 0;
					}
				}
			case WM_RBUTTONUP:
				{
					app & cat = app::instance();
					if( IsWindowVisible(cat.wnd_main) ) {
						const int restore = IsZoomed(cat.wnd_main) ? SW_SHOWMAXIMIZED : SW_RESTORE;
						BOOL mn = IsIconic(cat.wnd_main);
						ShowWindow(cat.wnd_main, mn ? restore : SW_HIDE);
						if( mn ) SetForegroundWindow(cat.wnd_main);
					} else {
						cat.ui_main_restore();
					}
				}
				return 0;
			}
			break;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, w_param, l_param);
}
