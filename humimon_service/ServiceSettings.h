#pragma once

#define SUBKEY L"SOFTWARE\\HumimonService"
#define ERROR -1
#define DEFAULT_URL L"http://192.168.33.10:4567/submit?st=%.4f&t=%.1f&h=%.1f&ts=%llu"
#define DEFAULT_INTERVAL 10
#define DEFAULT_SERIAL 512

class CServiceSettings {
public:
	DWORD Serial = DEFAULT_SERIAL, Interval = DEFAULT_INTERVAL;
	wchar_t URL[500];
	CServiceSettings();
	int save();
	void remove();
	
private:
	HKEY HumimonKey = NULL;
	wchar_t SubKey[255] = SUBKEY;
	int OpenKey();
	void CloseKey();
	int SetURL(PWSTR URL);
	int RemoveSettings(void);
	int SetSerialNumber(DWORD Serial);
	int SetInterval(DWORD Interval);
	int RegSaveValue(PWSTR name, DWORD dwType, LPBYTE data, DWORD size);	
	int ReadSettings(void);
	int CreateKey();
};
