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
	CURL *curl;
	CURLcode res;	
	LPWSTR ErrorText = new wchar_t[100];
	LPWSTR LogText = new wchar_t[100];
	littleWire *lw = NULL;
	HANDLE hTimer = NULL;
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -100000000LL; //10 sec
	hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == hTimer)
	{
		
		swprintf_s(ErrorText, 100, L"CreateWaitableTimer failed %d", GetLastError());
		WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
	}
		
	while (!m_fStopping)
	{			
		if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
		{
			
			swprintf_s(ErrorText, 100, L"SetWaitable timer failed %d", GetLastError());
			WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
		}

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) 
		{
			
			swprintf_s(ErrorText, 100,L"WaitForSingleObject failed %d", GetLastError());
			WriteEventLogEntry(ErrorText, EVENTLOG_ERROR_TYPE);
		}			
		else
		{
			WriteEventLogEntry(L"Timer tick", EVENTLOG_INFORMATION_TYPE);
			if (lw == NULL)
				lw = littleWire_connect();
			
			if (lw)
			{
				dht_reading val = dht_read(lw, DHT22);
				float temperature = TemperatureRead(lw);

				if ((!val.error || temperature != -404) && !littleWire_error())
				{

					curl = curl_easy_init();

					if (curl)
					{
						curl_easy_setopt(curl, CURLOPT_URL, "http://google.com");
						res = curl_easy_perform(curl);
						curl_easy_cleanup(curl);						
					}
					
					swprintf_s(LogText, 100, L"humidity: %f, temp %f, sensor: %f, res: %d", (float)val.humid / 10.0, (float)val.temp / 10.0, temperature, res);
					WriteEventLogEntry(LogText, EVENTLOG_INFORMATION_TYPE);
				}
				else
				{
					//SetDlgItemText(IDC_STATIC, L"Error Reading sensor!");
					//SetDlgItemText(IDC_STATIC, CA2W(littleWire_errorName()));

					lw = NULL;
				}
			}
			
		}	
		
	}

	SetEvent(m_hStoppedEvent);
}

float CHumimonService::TemperatureRead(littleWire* lw)
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

	float temperature = ((temphigh & 0x07) << 4);

	if (temphigh & (0x01 << 11))
		temperature = -temperature;

	for (int i = 0; i < 8; i++)
	{
		if (templow & (1 << i))
			temperature += powf(2, (i - 4));
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