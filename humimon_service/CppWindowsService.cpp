/****************************** Module Header ******************************\
* Module Name:  CppWindowsService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
*
* The file defines the entry point of the application. According to the
* arguments in the command line, the function installs or uninstalls or
* starts the service by calling into different routines.
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
#include <stdio.h>
#include <windows.h>
#include "ServiceInstaller.h"
#include "ServiceBase.h"
#include "ServiceSettings.h"
#include "humimon_service.h"
#include <tchar.h>
#include "littlewire.h"
#pragma endregion


// 
// Settings of the service
// 

// Internal name of the service
#define SERVICE_NAME             L"Humimon"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"Humidity and temperature logger"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""

// The name of the account under which the service should run
#define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"

// The password to the service account name
#define SERVICE_PASSWORD         NULL

void PrintHelp(void)
{
	wprintf(L"Parameters:\n");
	wprintf(L" -install  to install the service.\n");
	wprintf(L" -remove   to remove the service.\n");
	wprintf(L" -set-url [url] to set the remote logging service URL.\n");
	wprintf(L" -set-interval [seconds] to set the logging interval.\n");
	wprintf(L" -remove-settings to remove all settings from registry.\n");
	wprintf(L" -set-serial to set the LittleWire serial number. And select it for connection.\n");
}

//
//  FUNCTION: wmain(int, wchar_t *[])
//
//  PURPOSE: entrypoint for the application.
// 
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    wmain() either performs the command line task, or run the service.
//
int wmain(int argc, wchar_t *argv[])
{
	if ((argc > 1) && (*argv[1] == L'-'))
	{
		for (int i = 1; i < argc; i++)
		{
			bool install_option_unlocked = true;
			if (_wcsicmp(L"install", argv[i] + 1) == 0 && install_option_unlocked)
			{
				install_option_unlocked = false;
				// Install the service when the command is 
				// "-install" or "/install".
				InstallService(
					SERVICE_NAME,               // Name of service
					SERVICE_DISPLAY_NAME,       // Name to display
					SERVICE_START_TYPE,         // Service start type
					SERVICE_DEPENDENCIES,       // Dependencies
					SERVICE_ACCOUNT,            // Service running account
					SERVICE_PASSWORD            // Password of the account
				);
			}
			else if (_wcsicmp(L"remove", argv[i] + 1) == 0 && install_option_unlocked)
			{
				install_option_unlocked = false;
				// Uninstall the service when the command is 
				// "-remove" or "/remove".
				UninstallService(SERVICE_NAME);
				if (0 == RemoveSettings()) wprintf(L"Settings removed from registry\n");
			}
			else if (_wcsicmp(L"set-url", argv[i] + 1) == 0)
			{
				if (_tcslen(argv[i + 1]) > 0 && (*argv[i + 1] != L'-'))
				{
					wprintf(L"Setting URL to: %s\n", argv[i + 1]);
					SetURL(argv[i + 1]);
					i++;
				}
				else
				{
					wprintf(L"Please provide the URL\n");
				}
			}
			else if (_wcsicmp(L"set-interval", argv[i] + 1) == 0)
			{
				if (_tcslen(argv[i + 1]) > 0 && (*argv[i + 1] != L'-'))
				{
					DWORD Interval = _ttol(argv[i + 1]);
					wprintf(L"Setting interval to: %d\n", Interval);
					SetInterval(Interval);
					i++;
				}
			}
			else if (_wcsicmp(L"remove-settings", argv[i] + 1) == 0)
			{
				if (0 == RemoveSettings()) wprintf(L"Settings removed from registry\n");

			}
			else if (_wcsicmp(L"set-serial", argv[i] + 1) == 0)
			{
				int DeviceCount = littlewire_search();
				int Selection = -1;
				switch (DeviceCount)
				{
				case 0:
					wprintf(L"No LittleWire devices found\n");
					break;
				case 1:
					wprintf(L"Found 1 LittleWire device: [ %d ]\n", lwResults[0].serialNumber);
					Selection = 0;
					break;
				default:
					wprintf(L"Found %d LittleWire devices, please select the humimon device.\n\n", DeviceCount);
					for (int i = 0; i < DeviceCount; i++)
					{
						wprintf(L"\t[ %d ] Serial: %d\n", i + 1, lwResults[i].serialNumber);
					}
					
					while (1)
					{
						wprintf(L"[1]>");
						TCHAR DeviceNumber[100];
						if (NULL != _getts_s(DeviceNumber), 100)
						{

							if (_tcslen(DeviceNumber) == 0)
							{
								Selection = 0;
							}
							else
							{
								Selection = _ttoi(DeviceNumber) - 1;
							}
							if (0 <= Selection && Selection < DeviceCount)
							{
								wprintf(L"\n\t[ %d ] Serial: %d selected\n", Selection + 1, lwResults[Selection].serialNumber);
								break;
							}
							else {
								wprintf(L"%d is not available, please select other device\n", Selection + 1);
								Selection = -1;
							}
						}
					}


				}

				//selected device, now set the serial

				int NewSerialNumber = -1;				
				wprintf(L"Please enter new serial number for the selected device(100-999):\n");				
				while (1)
				{
					TCHAR Input[100];
					wprintf(L"[%d]>", lwResults[Selection].serialNumber);
				
					if (NULL != _getts_s(Input, 100))
					{

						if (_tcslen(Input) == 0)
						{
							NewSerialNumber = lwResults[Selection].serialNumber;
						}
						else
						{
							NewSerialNumber = _ttoi(Input);
						}
						if (100 <= NewSerialNumber && NewSerialNumber <= 999)
						{
							wprintf(L"Setting new serial number to %d\n", NewSerialNumber);
							littleWire* lw = NULL;
							lw = littlewire_connect_byID(Selection);
							if (lw != NULL) 
							{
								if (lwResults[Selection].serialNumber != NewSerialNumber)
								{
									changeSerialNumber(lw, NewSerialNumber);
									wprintf(L"Changed successfully, please reconnect the device.\n");
								}
								wprintf(L"Saving selected serial number to registry\n");
								SetSerialNumber(NewSerialNumber);
								
							} 
							else 
							{
								wprintf(L"Error connecting to the device: %s", littleWire_errorName());
							}

							break;
						}
						else {
							wprintf(L"Not in range 100..999\n");
							NewSerialNumber = -1;
						}
					}
				}

			}
			else
			{
				wprintf(L"Unknown option \"%s\"\n\n", argv[i]);
				PrintHelp();
			}
		}
	}
	else
	{
		PrintHelp();
		CHumimonService service(SERVICE_NAME);
		if (!CServiceBase::Run(service))
		{
			wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
		}
	}

	return 0;
}

