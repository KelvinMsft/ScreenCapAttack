// ScreenCap.h : main header file for the ScreenCap DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CScreenCapApp
// See ScreenCap.cpp for the implementation of this class
//

ULONG WINAPI InitCap();
ULONG WINAPI Capture();
class CScreenCapApp : public CWinApp
{
public:
	CScreenCapApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
