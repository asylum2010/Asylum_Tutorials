
#include "customcombo.h"
#include <Vssym32.h>

// http://msdn.microsoft.com/en-us/library/windows/desktop/hh404152%28v=vs.85%29.aspx
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd373487%28v=vs.85%29.aspx
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb773210%28v=vs.85%29.aspx

// http://www.codeguru.com/cpp/controls/buttonctrl/advancedbuttons/article.php/c5161
// http://www.windows-api.com/microsoft/Win32-UI/29257426/dropdown-list-with-cbsdropdownlist--cbsownerdrawvariable-in-vista.aspx

WNDPROC CustomCombo::originalproc = 0;
CustomCombo::ControlMap CustomCombo::combomap;

LRESULT CustomCombo::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// TODO: WM_PAINT, etc.

	ControlMap::iterator it = combomap.find(hwnd);
	static DWORD prev = 0;

	if (it != combomap.end()) {
		switch  (msg) {
		case WM_DESTROY:
			CloseThemeData(it->second->theme);
			break;
		}
	}

	return CallWindowProc(originalproc, hwnd, msg, wparam, lparam);
}

CustomCombo::CustomCombo()
{
	hwnd	= NULL;
	id		= 0;
	parent	= NULL;
	theme	= NULL;
}

CustomCombo::~CustomCombo()
{
}

bool CustomCombo::Initialize(int id, HWND parent, const RECT& rect)
{
	// from resource
	// hwnd = ::GetDlgItem(parent, id);
	
	hwnd = CreateWindow(
		"COMBOBOX",
		".",
		WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		parent,
		(HMENU)(0ll + id),
		(HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), 
		NULL);

	combomap[hwnd] = this;

	if (IsAppThemed())
		theme = OpenThemeData(hwnd, L"COMBOBOXSTYLE;COMBOBOX");

	if (!originalproc) {
		originalproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);

		if (!originalproc)
			return false;
	}

	if (SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)&CustomCombo::WndProc) == 0)
		return false;

	HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hwnd, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));

	this->parent = parent;
	this->id = (WORD)id;

	return true;
}

void CustomCombo::RenderItem(LPDRAWITEMSTRUCT ds)
{
	TCHAR		itemtext[256];
	TEXTMETRIC	tm;
	COLORREF	fg = 0, bg = 0;
	int			x, y, i = 0;
	int			textlen;

	// measure item
	GetTextMetrics(ds->hDC, &tm);

	y = (ds->rcItem.bottom + ds->rcItem.top - tm.tmHeight) / 2;
	x = LOWORD(GetDialogBaseUnits()) / 4;

	// get the item's text
	SendMessage(ds->hwndItem, CB_GETLBTEXT, ds->itemID, (LPARAM)itemtext);
	textlen = (int)strlen(itemtext);

	if (itemtext[0] == '@') {
		fg = SetTextColor(ds->hDC, GetSysColor(COLOR_GRAYTEXT));

		i = 1;
		--textlen;
	} else {
		bg = SetBkColor(
			ds->hDC, GetSysColor((ds->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW));

		fg = SetTextColor(
			ds->hDC, GetSysColor((ds->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
	}

	ExtTextOut(
		ds->hDC, 2 * x, y, ETO_CLIPPED|ETO_OPAQUE, &ds->rcItem,
		&itemtext[i], (UINT)textlen, NULL);

	// restore colors
	SetTextColor(ds->hDC, fg);

	if (i != 1)
		SetBkColor(ds->hDC, bg);

	// draw focus rect
	if (ds->itemState & ODS_FOCUS && i != 1)
		::DrawFocusRect(ds->hDC, &ds->rcItem);
}

bool CustomCombo::Render(LPDRAWITEMSTRUCT ds)
{
	if (ds->itemID == -1)
		return false;

	RenderItem(ds);
	return true;
}

LRESULT CustomCombo::ProcessCommands(WPARAM wparam, LPARAM lparam)
{
	HWND ctrl = (HWND)lparam;
	WORD msg = HIWORD(wparam);

	if (msg == CBN_SELCHANGE) {
		TCHAR itemtext[256];

		LRESULT ind = SendMessage(ctrl, CB_GETCURSEL, 0, 0);
		SendMessage(ctrl, CB_GETLBTEXT, ind, (LPARAM)itemtext);

		if (itemtext[0] == '@')
			SendMessage(ctrl, CB_SETCURSEL, cursel, 0);
		else
			cursel = (int)ind;
	}

	return 0;
}
