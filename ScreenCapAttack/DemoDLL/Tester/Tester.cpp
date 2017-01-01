// Tester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h" 
#include "Windows.h"
typedef LONG (WINAPI *CAPTURE)(); 
int main()
{
	HMODULE hmode = LoadLibrary(L"ScreenCap.dll");  
	CAPTURE Capture = (CAPTURE)GetProcAddress(hmode, "CopyScreen1");  
	CAPTURE Capture2 = (CAPTURE)GetProcAddress(hmode, "CopyScreen2");
	if (!Capture)
	{
		MessageBox(0, L"Cannot Load Library", 0, 0);
		return 0;
	}
	while (TRUE)
	{ 
		char c = getchar(); 
		if(c=='1')
		{
			Capture();
		}
		if (c == '2')
		{
			Capture2();
		}
	}
	return 0;
}

