#include "j_platform.h"

#include <Windows.h>
#include <Commdlg.h>
#include <stdio.h>
#include "types.h"
#include "j_assert.h"
#include "constants.h"

bool wide_str_to_str(WCHAR w_chars[], char chars[])
{
	int arraySize = wcslen(w_chars) + 1;

	for (int i = 0; i < arraySize; ++i)
	{
		int bufferSize = WideCharToMultiByte(CP_UTF8, 0, &w_chars[i], 1, NULL, 0, NULL, NULL);

		if (WideCharToMultiByte(CP_UTF8, 0, &w_chars[i], 1, &chars[i], bufferSize, NULL, NULL) == 0)
		{
			printf("Error converting WCHAR to char at index %d.\n", i);
			return false;
		}
	}

	return true;
}

bool file_dialog_get_filepath(char* file_path)
{
	OPENFILENAMEW ofn;
	WCHAR temp_wchars[FILE_PATH_LEN] = {};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
	ofn.lpstrFile = temp_wchars;
	ofn.nMaxFile = FILE_PATH_LEN;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	int success = GetOpenFileNameW(&ofn);
	if (!success) return false;

	success = wide_str_to_str(temp_wchars, file_path);
	return success;
}

bool file_dialog_make_filepath(char* file_path)
{
	OPENFILENAMEW ofn;
	WCHAR temp_wchars[FILE_PATH_LEN] = {};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
	ofn.lpstrFile = temp_wchars;
	ofn.nMaxFile = FILE_PATH_LEN;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	int success = GetSaveFileNameW(&ofn);
	if (!success) return false;

	success = wide_str_to_str(temp_wchars, file_path);
	return success;
}
