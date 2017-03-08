#pragma region "Includes"
#include <stdio.h>
#include <windows.h>
#include "ServiceSettings.h"
#include <tchar.h>
#include <sddl.h>
#pragma endregion

CServiceSettings::CServiceSettings() 
{
	wcscpy_s(URL, 500, DEFAULT_URL);
	if (SETTINGS_ERROR != CreateKey())
	{
		ReadSettings();
		CloseKey();
	}
}

int CServiceSettings::OpenKey()
{
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_READ, &HumimonKey))		
		return 0;
	return SETTINGS_ERROR;
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
	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = FALSE;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	ConvertStringSecurityDescriptorToSecurityDescriptor(L"D:AI(A; CI; KA;;; LS)"
		L"(A;ID;KR;;;BU)(A;CIIOID;GR;;;BU)"
		L"(A;ID;KA;;;BA)(A;CIIOID;GA;;;BA)"
		L"(A;ID;KA;;;SY)(A;CIIOID;GA;;;SY)", SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
										SubKey, 
										0, NULL, 0, KEY_ALL_ACCESS, 
										&sa, &HumimonKey, NULL)) return SETTINGS_ERROR;
	
	return SETTINGS_SUCCESS;
}
int CServiceSettings::save()
{
	CreateKey();
	if (HumimonKey == NULL) return SETTINGS_ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"Serial", 0, REG_DWORD, (LPBYTE)&Serial, sizeof(DWORD))) return SETTINGS_ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"Interval", 0, REG_DWORD, (LPBYTE)&Interval, sizeof(DWORD))) return SETTINGS_ERROR;
	if (ERROR_SUCCESS != RegSetValueEx(HumimonKey, L"URL", 0, REG_SZ,(LPBYTE)URL, wcslen(URL)*sizeof(wchar_t))) return SETTINGS_ERROR;
	CloseKey();
	return SETTINGS_SUCCESS;
}

void CServiceSettings::remove()
{
	RegDeleteKey(HKEY_LOCAL_MACHINE, SubKey);
}