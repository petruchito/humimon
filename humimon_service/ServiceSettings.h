#pragma once
int SetURL(PWSTR URL);
int RemoveSettings(void);
int SetSerialNumber(DWORD Serial);
int SetInterval(DWORD Interval);
int RegSaveValue(PWSTR name, DWORD dwType, LPBYTE data, DWORD size);
