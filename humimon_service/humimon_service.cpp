/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
*
* Provides a sample service class that derives from the service base class -
* CServiceBase. The sample service logs the service start and stop
* information to the Application event log, and shows how to run the main
* function of the service in a thread pool worker thread.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma region Includes
#include "humimon_service.h"
#include "ThreadPool.h"
#include "littleWire.h"
#include "littleWire_util.h"
#include "curl/curl.h"
#include <tchar.h>
#include <chrono>
#include "ServiceSettings.h"
#pragma endregion

CHumimonService::CHumimonService(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	m_fStopping = FALSE;

	// Create a manual-reset event that is not signaled at first to indicate 
	// the stopped signal of the service.
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hStoppedEvent == NULL)
	{
		throw GetLastError();
	}
}


CHumimonService::~CHumimonService(void)
{
	if (m_hStoppedEvent)
	{
		CloseHandle(m_hStoppedEvent);
		m_hStoppedEvent = NULL;
	}
}


//
//   FUNCTION: CHumimonService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void CHumimonService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
	// Log a service start message to the Application log.
	WriteEventLogEntry(L"Humimon service in OnStart",
		EVENTLOG_INFORMATION_TYPE);

	// Queue the main service function for execution in a worker thread.
	CThreadPool::QueueUserWorkItem(&CHumimonService::ServiceWorkerThread, this);
}

//
//   FUNCTION: CHumimonService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void CHumimonService::ServiceWorkerThread(void)
{
	int LastLoggedError = 0;
	CServiceSettings Settings;

	CURL *curl;
	CURLcode res;
	TCHAR ErrorText[100];
	TCHAR LogText[100];
	littleWire *lw = NULL;
	HANDLE hTimer = NULL;

	LARGE_INTEGER liDueTime;
	//liDueTime.QuadPart = -100000000LL; //10 sec in 0.1us intervals
	liDueTime.QuadPart = Settings.Interval * -10000000LL;
	hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	
	if (NULL == hTimer)
	{
		_stprintf_s(ErrorText, 100, _T("CreateWaitableTimer failed %d"), GetLastError());
		WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
	}

	while (!m_fStopping)
	{
		if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
		{
			_stprintf_s(ErrorText, 100, _T("SetWaitable timer failed %d"), GetLastError());
			WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
		}

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
		{

			_stprintf_s(ErrorText, 100, _T("WaitForSingleObject failed %d"), GetLastError());
			WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
		}
		else
		{
			if (lw == NULL)
			{
				littlewire_search();
				lw = littlewire_connect_bySerialNum(Settings.Serial);
			}
			CServiceSettings SettingsTest;
			if (lw)
			{
				LastLoggedError = 0;
				dht_reading val = dht_read(lw, DHT22);
				double temperature = TemperatureRead(lw);

				if ((!val.error || temperature != -404) && !littleWire_error())
				{
					curl = curl_easy_init();
					if (curl)
					{
						char URLString[500] = { 0 };
						char URLTemplate[500] = { 0 };
						size_t converted = 0;
						//TODO add a registry value for URL format string
						wcstombs_s(&converted,URLTemplate, Settings.URL, 500);
						//check conv length, make movable args in template
						//"http://192.168.33.10:4567/submit?st=%.4f&t=%.1f&h=%.1f&ts=%llu"
						sprintf_s(URLString, 500, URLTemplate,
							temperature,
							(float)val.temp / 10,
							(float)val.humid / 10,
							std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

						curl_easy_setopt(curl, CURLOPT_URL, URLString);
						res = curl_easy_perform(curl);
						if (res)
						{
							_stprintf_s(LogText, 100, _T("Curl result: %d"), res);
							WriteEventLogEntry(LogText, EVENTLOG_ERROR_TYPE);
						}

						curl_easy_cleanup(curl);
					}
				}
				else
				{
					TCHAR* Errors[3];

					if (temperature == -404) Errors[0] = _T("Error reading DS18B20");
					switch (val.error)
					{
					case 0:
						break;
					case 128:
						Errors[1] = _T("DHT timeout error");
						break;
					case 255:
						Errors[2] = _T("DHT checksum error");
						break;
					}
					for (char i = 0; i < 3; i++)
					{
						WriteEventLogEntry(Errors[i], EVENTLOG_ERROR_TYPE);						
					}
					if (littleWire_error()) {
						LogLittleWireError();
						lw = NULL;
					}
					//LastLoggedError = error;
					
				}
			} //if (lw)
			else
			{
				int error = littleWire_error();
				if (error != LastLoggedError)
				{
					LogLittleWireError();
					LastLoggedError = error;
				}
			}

		}

		SetEvent(m_hStoppedEvent);
	}
}

void CHumimonService::LogLittleWireError(void)
{
	char* lwErrorNameA = littleWire_errorName();
	TCHAR LogText[255];
#ifdef UNICODE
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, lwErrorNameA, -1, NULL, 0);
	wchar_t* lwErrorName = new wchar_t[wchars_num];
	MultiByteToWideChar(CP_UTF8, 0, lwErrorNameA, -1, lwErrorName, wchars_num);
	swprintf_s(LogText, 255, L"Littlewire error: %s", lwErrorName);
	WriteEventLogEntry(LogText, EVENTLOG_ERROR_TYPE);
	delete[] lwErrorName;
#endif // UNICODE

#ifndef UNICODE
	sprintf_s(LogText, 100, "Littlewire error: %s", lwErrorName);
#endif // !UNICODE

}

double CHumimonService::TemperatureRead(littleWire* lw)
{
	unsigned char scratch, temphigh, templow = 0;

	if (!onewire_resetPulse(lw)) return -404;
	onewire_writeByte(lw, 0xCC); // skip rom
	onewire_writeByte(lw, 0x44); // convert T command
	::Sleep(750); // wait for conversion

	if (!onewire_resetPulse(lw)) return -404;
	onewire_writeByte(lw, 0xCC); // skip rom
	onewire_writeByte(lw, 0xBE); // read scratchpad

	for (int i = 0; i<9; i++) //read 9 bytes from SCRATCHPAD
	{
		scratch = onewire_readByte(lw);

		switch (i)
		{
		case 0:
			templow = scratch;
		case 1:
			temphigh = scratch;
		}
	}

	double temperature = ((temphigh & 0x07) << 4);

	if (temphigh & (0x01 << 11))
		temperature = -temperature;

	for (int i = 0; i < 8; i++)
	{
		if (templow & (1 << i))
			temperature += pow(2, (i - 4)); 
	}

	return temperature;
}

//
//   FUNCTION: CHumimonService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void CHumimonService::OnStop()
{
	// Log a service stop message to the Application log.
	WriteEventLogEntry(L"Humimon service in OnStop",
		EVENTLOG_INFORMATION_TYPE);

	// Indicate that the service is stopping and wait for the finish of the 
	// main service function (ServiceWorkerThread).
	m_fStopping = TRUE;
	if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	{
		throw GetLastError();
	}
}