
#ifndef _CUSTOMCOMBO_H_
#define _CUSTOMCOMBO_H_

#include <Windows.h>
#include <Uxtheme.h>
#include <map>

class CustomCombo
{
protected:
	WORD	id;
	HWND	hwnd;
	HWND	parent;
	HTHEME	theme;
	int		cursel;

	// TODO: inherit from abstract Control class
	typedef std::map<HWND, CustomCombo*> ControlMap;
	static ControlMap combomap;

	static WNDPROC originalproc;
	static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void RenderItem(LPDRAWITEMSTRUCT ds);

public:
	CustomCombo();
	~CustomCombo();

	bool Initialize(int id, HWND parent, const RECT& rect);
	bool Render(LPDRAWITEMSTRUCT ds);

	LRESULT ProcessCommands(WPARAM wparam, LPARAM lparam);

	inline int AddString(LPCTSTR lpszString) const {
		return (int)SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)lpszString);
	}

	inline int DeleteString(int nIndex) const {
		return (int)SendMessage(hwnd, CB_DELETESTRING, (WPARAM)nIndex, 0);
	}

	inline int GetCount() const {
		return (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0);
	}

	inline void SetCurSel(int index) {
		cursel = index;
		SendMessage(hwnd, CB_SETCURSEL, (WPARAM)index, 0L);
	}

	inline void ShowWindow(int show) {
		::ShowWindow(hwnd, show);
	}

	inline LRESULT GetCurSel() {
		return SendMessage(hwnd, CB_GETCURSEL, 0L, 0L);
	}

	inline HWND GetHandle() {
		return hwnd;
	}

	inline HWND GetParent() {
		return parent;
	}

	inline WORD ID() const {
		return id;
	}
};

#endif
