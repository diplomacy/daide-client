// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but are changed infrequently

// Copyright (C) 2012, John Newbury. See "Conditions of Use" in johnnewbury.co.cc/diplomacy/conditions-of-use.htm.

// Release 8~2~b

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <windows.h>
#include <cstdlib>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <ctime>
#include <queue>
#include "crtdbg.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <set>
#include <list>
#include <map>
#include <string>

using namespace std;

namespace JPN {}

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define ASSERT(x) while (!(x) && _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, "", #x )) {}
#else
#define ASSERT(x)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
