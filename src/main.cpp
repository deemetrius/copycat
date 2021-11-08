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

	// main menu
	if(( cat.menu_main = CreateMenu() )) {
		AppendMenu(cat.menu_main, MF_ENABLED | MF_STRING, app::id_save, _T("Sa&ve") );
		AppendMenu(cat.menu_main, MF_ENABLED | MF_STRING, app::id_load, _T("&Load") );
		AppendMenu(cat.menu_main, MF_ENABLED | MF_STRING | MFT_RIGHTJUSTIFY, app::id_about, _T("Abou&t") );
		AppendMenu(cat.menu_main, MF_ENABLED | MF_STRING, app::id_exit, _T("&Quit") );
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
		cat.menu_main, // menu
		h_inst, // instance handler
		NULL // no creation data
	);
	if( !cat.wnd_main ) {
		log(L"error: CreateWindowEx main");
		return 0;
	}

	//
	cat.on_create(h_inst);
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

	cat.is_list_ready = true;

	// msg loop
	MSG messages;
	while( GetMessage(&messages, NULL, 0, 0) ) {
		if( !IsDialogMessage(cat.wnd_main, &messages) ) {
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		}
	}

	// write config
	cat.data.write();

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
	case WM_NOTIFY:
		{
			LPNMHDR inf_short = (LPNMHDR) l_param;
			switch( inf_short->idFrom ) {
			case id_list:
				switch( inf_short->code ) {
				case LVN_ITEMCHANGED:
					{
						LPNMLISTVIEW inf = (LPNMLISTVIEW) l_param;
						if( inf->iItem >= 0 /*&& inf->uNewState & LVIS_SELECTED*/ ) {
							app & cat = app::instance();
							/*LVHITTESTINFO hit{};
							hit.pt = inf->ptAction;
							ListView_HitTest(cat.wnd_list, &hit);
							if( !(hit.flags & LVHT_ONITEMLABEL) ) {*/
							if( inf->uChanged & LVIF_STATE && !(inf->uNewState & LVIS_SELECTED) ) {
								cat.on_list_check(inf->iItem);
							} else if( ListView_GetItemState(cat.wnd_list, inf->iItem, LVIS_SELECTED) ) {
								cat.on_list_get(inf->iItem);
								return 0;
							}
						}
					}
					break;
				case NM_CLICK:
				case NM_RCLICK:
					{
						LPNMITEMACTIVATE inf = (LPNMITEMACTIVATE) l_param;
						if( inf->iItem >= 0 /*&& inf->uNewState & LVIS_SELECTED*/ ) {
							app & cat = app::instance();
							LVHITTESTINFO hit{};
							hit.pt = inf->ptAction;
							ListView_HitTest(cat.wnd_list, &hit);
							if( !(hit.flags & LVHT_ONITEMLABEL) ) {
								cat.on_list_check(inf->iItem);
							} else if( ListView_GetItemState(cat.wnd_list, inf->iItem, LVIS_SELECTED) ) {
								cat.on_list_get(inf->iItem);
								return 0;
							}
						}
					}
					break;
				case LVN_KEYDOWN:
					{
						LPNMLVKEYDOWN inf = (LPNMLVKEYDOWN) l_param;
						switch( inf->wVKey ) {
						case VK_F2:
							{
								app & cat = app::instance();
								if(
									LRESULT res; ListView_GetSelectedCount(cat.wnd_list) &&
									(res = ListView_GetNextItem(cat.wnd_list, -1, LVNI_SELECTED) ) >= 0
								) {
									ListView_EditLabel(cat.wnd_list, res);
									return 0;
								}
							}
							break;
						} // switch
					}
					break;
				case LVN_ENDLABELEDIT:
					{
						NMLVDISPINFO * inf = (NMLVDISPINFO *) l_param;
						if( inf->item.pszText ) {
							text tx(ex::n_copy::val, inf->item.pszText, text::traits::length(inf->item.pszText) );
							if(( tx = ex::actions_text::trim(tx) )) {
								app & cat = app::instance();
								t_data::t_item & item = cat.data.items[inf->item.iItem];
								item.name = tx;
								item.make_combined();
								ListView_SetItemText(cat.wnd_list, inf->item.iItem, 0, tx->s_);
								cat.is_data_changed = true;
								return FALSE;
							}
						}
					}
					break;
				case NM_RETURN:
					{
						app & cat = app::instance();
						SetFocus(cat.wnd_name);
					}
					return 0;
				} // switch code
				break;
			} // switch idFrom
		}
		break;
	case WM_COMMAND:
		switch( /*WORD code =*/ HIWORD(w_param) ) {
		case 0:
		case 1:
		//case BN_CLICKED:
			switch( WORD id_c = LOWORD(w_param) ) {
			case id_exit:
				PostQuitMessage(0);
				return 0;
			case id_load:
				{
					app & cat = app::instance();
					cat.is_list_ready = false;
					ListView_DeleteAllItems(cat.wnd_list);
					cat.data.read();
					cat.make_menu();
					cat.is_list_ready = true;
				}
				return 0;
			case id_save:
				{
					app & cat = app::instance();
					cat.data.write();
				}
				return 0;
			case id_edit:
				{
					app & cat = app::instance();
					cat.on_main_restore();
				}
				return 0;
			case id_name_label:
				{
					app & cat = app::instance();
					SetFocus(cat.wnd_name);
				}
				return 0;
			case id_clear:
				{
					app & cat = app::instance();
					SetWindowText(cat.wnd_name, empty_str);
					SetWindowText(cat.wnd_value, empty_str);
				}
				return 0;
			case id_set:
				{
					app & cat = app::instance();
					cat.on_list_set();
				}
				return 0;
			case id_add:
				{
					app & cat = app::instance();
					cat.on_list_add();
				}
				return 0;
			case id_del:
				{
					app & cat = app::instance();
					cat.on_list_del();
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
								cat.on_main_restore();
								cat.on_list_sel(id_go);
								SetFocus(cat.wnd_list);
								return 0;
							}
						} // switch
					}
				}
			} // switch id_c
			break;
		} // switch code
		break;
	case WM_SIZE:
		{
			app & cat = app::instance();
			RECT rc;
			GetClientRect(cat.wnd_main, &rc);
			t_name::resize(&cat, rc);
			t_clear::resize(&cat, rc);
			t_value::resize(&cat, rc);
			t_list::resize(&cat, rc);
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
						cat.make_menu_only();
						TrackPopupMenuEx(
							cat.menu_tray,
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
						cat.on_main_restore();
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
