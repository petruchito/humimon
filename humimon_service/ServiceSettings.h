#pragma once

#define SUBKEY L"SOFTWARE\\HumimonService"
#define SETTINGS_ERROR -1
#define SETTINGS_SUCCESS 0
#define DEFAULT_URL L"http://192.168.33.10:4567/submit?st=%.4f&t=%.1f&h=%.1f&ts=%llu"
#define DEFAULT_INTERVAL 10
#define DEFAULT_SERIAL 512

class CServiceSettings {
public:
	DWORD Serial = DEFAULT_SERIAL, 
		  Interval = DEFAULT_INTERVAL;
	wchar_t URL[500];

	CServiceSettings();
	int save();
	void remove();
	
private:
	HKEY HumimonKey = NULL;
	wchar_t SubKey[255] = SUBKEY;
	int OpenKey();
	void CloseKey();
	int ReadSettings();
	int CreateKey();
};
