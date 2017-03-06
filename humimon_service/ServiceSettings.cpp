#pragma region "Includes"
#include <stdio.h>
#include <windows.h>
#include "ServiceSettings.h"
#include <tchar.h>
#pragma endregion

#define SUBKEY "SOFTWARE\\HumimonService"

int SetSerialNumber(DWORD Serial)
{
	RegSaveValue(L"Serial", REG_DWORD, (LPBYTE)&Serial, sizeof(DWORD));
	return 0;
}

int SetInterval(DWORD Interval)
{
	RegSaveValue(L"Interval", REG_DWORD, (LPBYTE)&Interval, sizeof(DWORD));
	return 0;
}

int SetURL(PWSTR URL)
{
	RegSaveValue(L"URL", REG_SZ, (LPBYTE)URL, _tcslen(URL)*sizeof(TCHAR));
	return 0;
}

int RemoveSettings(void)
{
	HKEY URLKey;
	TCHAR SubKey[100] = _T(SUBKEY);
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, &URLKey))
	{
		wprintf(_T("Settings key does not exist\n"));
		return 1;
	}
	if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, SubKey))
	{
		//FORMAT MESSAGE HERE
		wprintf(_T("Error removing the key\n"));
		return -1;
	}
	RegCloseKey(URLKey);
	return 0;
}

int RegSaveValue(PWSTR name, DWORD dwType, LPBYTE data, DWORD size)
{
	HKEY URLKey;
	TCHAR SubKey[100] = _T(SUBKEY);

	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &URLKey, NULL))
	{
		wprintf(_T("Error creating key\n"));
		return -1;
	}
	if (ERROR_SUCCESS != RegSetValueEx(URLKey, name, 0, dwType, data, size))
	{
		wprintf(_T("Error creating value\n"));
		return -1;

	}
	RegCloseKey(URLKey);

}