/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "common/setup_before.h"
#ifndef HAVE_UNAME
#include "uname.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef HAVE_WINDOWS_H
#include <Windows.h>
#define STATUS_SUCCESS (0x00000000)
#endif

#include "common/setup_after.h"


namespace pvpgn
{
	int uname(struct utsname* buf)
	{
		if (!buf)
		{
			errno = EFAULT;
			return -1;
		}

#ifdef HAVE_WINDOWS_H
		using RtlGetVersionProto = NTSTATUS(WINAPI*)(RTL_OSVERSIONINFOEXW* lpVersionInformation);

		HMODULE hNtdll = nullptr;
		if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"ntdll.dll", &hNtdll) != 0)
		{
			auto fnRtlGetVersion = reinterpret_cast<RtlGetVersionProto>(GetProcAddress(hNtdll, "RtlGetVersion"));
			if (fnRtlGetVersion != NULL)
			{
				RTL_OSVERSIONINFOEXW verinfo = {};
				verinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
				if (fnRtlGetVersion(&verinfo) == STATUS_SUCCESS)
				{
					std::string temp;

					SYSTEM_INFO sysinfo = {};
					GetSystemInfo(&sysinfo);

					switch (verinfo.dwMajorVersion)
					{
					case 10:
					{
						if (verinfo.wProductType == VER_NT_WORKSTATION)
						{
							temp.append("Windows 10");
						}
						else
						{
							temp.append("Windows Server 2016");
						}

						break;
					}
					case 6:
					{
						switch (verinfo.dwMinorVersion)
						{
						case 3:
						{
							if (verinfo.wProductType == VER_NT_WORKSTATION)
							{
								temp.append("Windows 8.1");
							}
							else
							{
								temp.append("Windows Server 2012 R2");
							}

							break;
						}
						case 2:
						{
							if (verinfo.wProductType == VER_NT_WORKSTATION)
							{
								temp.append("Windows 8");
							}
							else
							{
								temp.append("Windows Server 2012");
							}

							break;
						}
						case 1:
						{
							if (verinfo.wProductType == VER_NT_WORKSTATION)
							{
								temp.append("Windows 7");
							}
							else
							{
								temp.append("Windows Server 2008 R2");
							}

							break;
						}
						case 0:
						{
							if (verinfo.wProductType == VER_NT_WORKSTATION)
							{
								temp.append("Windows Vista");
							}
							else
							{
								temp.append("Windows Server 2008");
							}

							break;
						}
						default:
						{
							break;
						}
						}

						break;
					}
					case 5:
					{
						switch (verinfo.dwMinorVersion)
						{
						case 2:
						{
							if (GetSystemMetrics(SM_SERVERR2) != 0)
							{
								temp.append("Windows Server 2003 R2");
							}
							else if (verinfo.wSuiteMask & VER_SUITE_WH_SERVER)
							{
								temp.append("Windows Home Server");
							}
							else if (GetSystemMetrics(SM_SERVERR2) == 0)
							{
								temp.append("Windows Server 2003");
							}
							else if ((verinfo.wProductType == VER_NT_WORKSTATION) && (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64))
							{
								temp.append("Windows XP Professional x64 Edition");
							}

							break;
						}
						case 1:
						{
							temp.append("Windows XP");
							break;
						}
						case 0:
						{
							temp.append("Windows 2000");
							break;
						}
						default:
						{
							break;
						}
						}

						break;
					}
					default:
					{
						break;
					}
					}

					if (temp.empty())
					{
						temp.append("Windows");
					}

					if (verinfo.wServicePackMajor != 0)
					{
						temp.append(" SP" + std::to_string(verinfo.wServicePackMajor));
						if (verinfo.wServicePackMinor != 0)
						{
							temp.append("." + std::to_string(verinfo.wServicePackMinor));
						}
					}

					std::snprintf(buf->sysname, sizeof buf->sysname, "%s", temp.c_str());

					DWORD len = sizeof buf->nodename;
					GetComputerNameA(buf->nodename, &len);

					std::snprintf(buf->release, sizeof buf->release, "Build %lu", verinfo.dwBuildNumber);
					
					std::snprintf(buf->version, sizeof buf->version, "%lu.%lu", verinfo.dwMajorVersion, verinfo.dwMinorVersion);

					std::string arch;
					switch (sysinfo.wProcessorArchitecture)
					{
					case PROCESSOR_ARCHITECTURE_AMD64:
					{
						arch = "x64";
						break;
					}
					case PROCESSOR_ARCHITECTURE_ARM:
					{
						arch = "ARM";
						break;
					}
					case PROCESSOR_ARCHITECTURE_IA64:
					{
						arch = "Intel Itanium-based";
						break;
					}
					case PROCESSOR_ARCHITECTURE_INTEL:
					{
						arch = "x86";
						break;
					}
					case PROCESSOR_ARCHITECTURE_UNKNOWN:
					default:
						arch = "Unknown";
					}

					std::snprintf(buf->machine, sizeof buf->machine, "%s", arch.c_str());

					// leave this empty for now
					std::snprintf(buf->domainname, sizeof buf->domainname, "");
				}
			}
		}
#else // !HAVE_WINDOWS_H
		std::snprintf(buf->sysname, sizeof buf->sysname, "Unknown");
		std::snprintf(buf->nodename, sizeof buf->nodename, "Unknown");
		std::snprintf(buf->release, sizeof buf->release, "Unknown");
		std::snprintf(buf->version, sizeof buf->version, "Unknown");
		std::snprintf(buf->machine, sizeof buf->machine, "Unknown");
		std::snprintf(buf->domainname, sizeof buf->domainname, "Unknown");
#endif // HAVE_WINDOWS_H

		return 0;
	}

}
#endif
