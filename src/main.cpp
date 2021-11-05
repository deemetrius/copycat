#include "main.h"

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

	cat.dlu.rescale();
	// main window
	cat.wnd_main = CreateWindowEx(
		WS_EX_CONTROLPARENT | WS_EX_APPWINDOW, // ex_style // WS_EX_TOOLWINDOW
		wnd_class_name, // wnd_class
		_T("copycat"), // title
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SIZEBOX, // style // WS_BORDER
		CW_USEDEFAULT, // pos_x
		CW_USEDEFAULT, // pos_y
		400, // width
		400, //dlu.space_margin_y * 2 + size_list_y + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME /*SM_CYFIXEDFRAME*/) * 2, // height
		HWND_DESKTOP, // parent
		NULL, // menu
		h_inst, // instance handler
		NULL // no creation data
	);
	if( !cat.wnd_main ) {
		log(L"error: CreateWindowEx main");
		return 0;
	}

	//
	cat.ui_create(h_inst);
	cat.make_menu();
	ShowWindow(cat.wnd_main, SW_SHOWNORMAL);
	SetFocus(cat.wnd_name);
	ShowWindow(cat.wnd_main, SW_HIDE);

	// tray icon
	NOTIFYICONDATA nid{};
	nid.cbSize = sizeof(nid);
	nid.uID = app::id_tray;
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
		switch( /*WORD code =*/ HIWORD(w_param) ) {
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
						} // switch
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
			GetClientRect(cat.wnd_main, &rc);
			t_name::resize(cat.dlu, rc, cat.wnd_name);
			t_value::resize(cat.dlu, rc, cat.wnd_value);
			t_list::resize(cat.dlu, rc, cat.wnd_list);
			/*cat.ui_list_rect(rc);
			MoveWindow(cat.wnd_list, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);*/
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
