
#include "win32openfiledialog.h"

static void Split(std::vector<std::string>& out, const std::string& separators, const std::string& str, bool keepempty)
{
	out.clear();
	out.reserve(5);

	size_t start = 0;
	size_t pos = 0;

	do {
		pos = str.find_first_of(separators.c_str(), start);

		if (keepempty || pos - start > 0) {
			if (out.capacity() == out.size())
				out.reserve(out.size() + 5);

			out.push_back(str.substr(start, pos - start));
		}

		start = str.find_first_not_of(separators.c_str(), pos);
	} while (start != std::string::npos && pos != std::string::npos);
}

static std::string& Replace(std::string& out, char what, char with, const std::string& str)
{
	out.resize(str.length());

	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == what)
			out[i] = with;
		else
			out[i] = str[i];
	}

	return out;
}

static std::string& Crop(std::string& out, const std::string& str, char ch)
{
	size_t i = 0;
	size_t j = str.length() - 1;

	for (; str[i] == ch; ++i)
		;

	for (; str[j] == ch; --j)
		;

	out = str.substr(i, (j - i) + 1);
	return out;
}

Win32OpenFileDialog::Win32OpenFileDialog()
{
	memset(&ofn, 0, sizeof(OPENFILENAMEW));

	filter = "All Files (*.*)|*.*||";
		
	InitialDirectory = "./";

	ofn.lStructSize				= sizeof(OPENFILENAMEW);
	ofn.lpstrCustomFilter		= 0;
	ofn.nMaxCustFilter			= 0;
	ofn.nFilterIndex			= 0;
	ofn.nMaxFile				= SHRT_MAX;
	ofn.lpstrFileTitle			= 0;
	ofn.nMaxFileTitle			= 0;
	ofn.Flags					= OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
	ofn.nFileOffset				= 0;
	ofn.nFileExtension			= 0;
	ofn.lpstrDefExt				= 0;
	ofn.lCustData				= 0;
	ofn.lpfnHook				= 0;
	ofn.lpTemplateName			= 0;
}

Win32OpenFileDialog::~Win32OpenFileDialog()
{
}

void Win32OpenFileDialog::AddFilter(const std::string& name, const std::string& ext)
{
	std::string tmp;

	if (ext[0] == '@') {
		std::vector<std::string> exts;
		std::string tmp2;

		Split(exts, "@;", ext, false);

		for (size_t i = 0; i < exts.size(); ++i)
			tmp2 += ("*." + exts[i] + ";");

		tmp = name + " (" + tmp2;
		tmp.back() = ')';

		tmp += '|';

		tmp += tmp2;
		tmp.back() = '|';
	} else {
		tmp = name + " (*." + ext + ")|*." + ext + "|";
	}

	filter = tmp + filter;
}

DialogResult Win32OpenFileDialog::Show(void* parent, bool multiselect)
{
	std::string tmp;
	int written;

	Replace(tmp, '|', '\0', filter);
		
	std::wstring wname;
	std::wstring wdir;
	std::wstring wfilter;
	std::wstring wdefext;

	wfilter.resize(512);
	wname.resize(SHRT_MAX);
	wdir.resize(SHRT_MAX);
	wdefext.resize(DefaultExtension.length());

	written = MultiByteToWideChar(CP_UTF8, 0, tmp.c_str(), (int)tmp.length(), &wfilter[0], (int)wfilter.length());

	if (written == 0)
		return DialogResultError;

	wfilter[written] = '\0';
	written = MultiByteToWideChar(CP_UTF8, 0, InitialDirectory.c_str(), (int)InitialDirectory.length(), &wdir[0], (int)wdir.length());
		
	if (written > 0)
		wdir[written] = '\0';

	ofn.hwndOwner		= (HWND)parent;
	ofn.lpstrFilter		= wfilter.c_str();
	ofn.lpstrFile		= &wname[0];
	ofn.lpstrInitialDir	= wdir.c_str();
	ofn.lpstrDefExt		= 0;
		
	if (multiselect)
		ofn.Flags |= OFN_ALLOWMULTISELECT|OFN_EXPLORER;

	if (DefaultExtension.length() > 0) {
		MultiByteToWideChar(CP_UTF8, 0, DefaultExtension.c_str(), (int)DefaultExtension.length(), &wdefext[0], (int)wdefext.length());
		ofn.lpstrDefExt = wdefext.c_str();
	}

	BOOL result = GetOpenFileNameW(&ofn);
	std::string file;

	FileNames.clear();

	if (result == FALSE) {
		if (FNERR_BUFFERTOOSMALL == CommDlgExtendedError()) {
			// TODO:
			return DialogResultError;
		}
	} else {
		if (multiselect) {
			std::wstring name;
			std::string dir;
			size_t loc = wname.find_first_of((wchar_t)0, 0);
			size_t next;

			// directory is first
			name = wname.substr(0, loc);
			dir.resize(SHRT_MAX, '\0');

			written = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), (int)name.length(), &dir[0], (int)dir.length(), NULL, NULL);
			Crop(dir, dir, '\0');

			dir += '\\';

			// then file names
			while (loc != std::wstring::npos) {
				next = wname.find_first_of((wchar_t)0, loc + 1);

				if (next - loc > 1) {
					name = wname.substr(loc + 1, next - loc);
					file.resize(SHRT_MAX, '\0');

					written = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), (int)name.length(), &file[0], (int)file.length(), NULL, NULL);
					Crop(file, file, '\0');

					FileNames.push_back(dir + file);
					loc = next;
				} else {
					loc = std::wstring::npos;
				}
			}

			if (FileNames.size() == 0) {
				// only 1 file selected
				dir.pop_back();
				FileNames.push_back(dir);
			}

			return DialogResultOk;
		} else {
			file.resize(SHRT_MAX, '\0');

			written = WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), (int)wname.length(), &file[0], (int)file.length(), NULL, NULL);
			Crop(file, file, '\0');

			FileNames.push_back(file);
		}

		return DialogResultOk;
	}

	return DialogResultCancel;
}
