
#ifndef _WIN32OPENFILEDIALOG_H_
#define _WIN32OPENFILEDIALOG_H_

#include <string>
#include <vector>
#include <Windows.h>

enum DialogResult
{
	DialogResultUnknown = 0,
	DialogResultOk,
	DialogResultCancel,
	DialogResultError
};

class Win32OpenFileDialog
{
protected:
	std::string filter;
	OPENFILENAMEW ofn;

public:
	std::vector<std::string> FileNames;
	std::string InitialDirectory;
	std::string DefaultExtension;

	Win32OpenFileDialog();
	~Win32OpenFileDialog();

	void AddFilter(const std::string& name, const std::string& ext);
	DialogResult Show(void* parent, bool multiselect = false);
};

#endif
