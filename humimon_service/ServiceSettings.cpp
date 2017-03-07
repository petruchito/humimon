#pragma region "Includes"
#include <stdio.h>
#include <windows.h>
#include "ServiceSettings.h"
#include <tchar.h>
#pragma endregion



CServiceSettings::CServiceSettings() 
{
	wcscpy_s(URL, 500, DEFAULT_URL);
	if (ERROR != CreateKey())
	{
		ReadSettings();
		CloseKey();
	}
}

int CServiceSettings::OpenKey()
{
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, &HumimonKey))		
		return 0;
	return ERROR;
}

void CServiceSettings::CloseKey()
{
	if (NULL != HumimonKey) RegCloseKey(HumimonKey);
	HumimonKey = NULL;
}

int CServiceSettings::ReadSettings()
{
	int index=0;
	wchar_t value[500];
	wchar_t value_name[255];
	DWORD value_size = wcslen(value) * sizeof(wchar_t);
	DWORD value_name_size = wcslen(value_name) * sizeof(wchar_t);
	DWORD value_type;
	DWORD vs = value_size,
		  vns = value_name_size;
	while (ERROR_SUCCESS == RegEnumValue(HumimonKey, index, value_name, &value_name_size, NULL, &value_type, (LPBYTE)&value, &value_size))
	{
		if (_wcsicmp(value_name, L"URL") == 0)
		{
			if (value_size != 0) wcscpy_s(URL, 500, value);
		}
		else if (_wcsicmp(value_name, L"Interval") == 0)
		{
			if (value_size != 0) Interval = (DWORD) *value;
		}
		else if (_wcsicmp(value_name, L"Serial") == 0)
		{
			if (value_size != 0) Serial = (DWORD) *value;
		}
		index++;
		value_size = vs;
		value_name_size = vns;
	}

	return 1;
}

int CServiceSettings::CreateKey()
{
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
										SubKey, 
										0, NULL, 0, KEY_ALL_ACCESS, 
										NULL, &HumimonKey, NULL)) return ERROR;	
	return 0;
}
int CServiceSettings::save()
{
	CreateKey();
	if (HumimonKey == NULL) return ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"Serial", 0, REG_DWORD, (LPBYTE)&Serial, sizeof(DWORD))) return ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"Interval", 0, REG_DWORD, (LPBYTE)&Interval, sizeof(DWORD))) return ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"URL", 0, REG_SZ,(LPBYTE)URL, wcslen(URL)*sizeof(wchar_t))) return ERROR;
	CloseKey();
}

void CServiceSettings::remove()
{
	RegDeleteKey(HKEY_LOCAL_MACHINE, SubKey);

}

int CServiceSettings::RegSaveValue(PWSTR name, DWORD dwType, LPBYTE data, DWORD size)
{
	HKEY URLKey;
	TCHAR SubKey[100] = SUBKEY;

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
