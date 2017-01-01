// ScreenCap.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "resource.h"
#include "ScreenCap.h" 
#include <newdev.h> // for the API UpdateDriverForPlugAndPlayDevices().
#include <Windows.h>
#include <Setupapi.h>
#include <shlobj.h>
#include "Magnification.h"
#include "HostDialog.h"
#include "tlhelp32.h"


HostDialog* dlg;
#pragma comment (lib, "newdev.lib")
#pragma comment (lib, "shell32.lib")
#pragma comment (lib, "Setupapi.lib")


///////////////////////////////////////////////////////////////////////////////////
//// Enumeration
////

enum ReleaseError
{
	INVALID_DLL_PATH = 0,
	INVALID_RES_NAME,
	CANNOT_LOAD_RES,
	CANNOT_LOCK_RES,
	CANNOT_CREATE_FILE,
	CANNOT_WRITE_FAILE,
	EVERYTHING_ALRIGHT,
};

///////////////////////////////////////////////////////////////////////////////////
//// Marco
////
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifndef arraysize
#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif


#define COMMON_MODE DM_BITSPERPEL | DM_PELSWIDTH  |  DM_PELSHEIGHT | DM_POSITION
#define BITMAP_MAGIC 0x4d42
BEGIN_MESSAGE_MAP(CScreenCapApp, CWinApp)
END_MESSAGE_MAP()
 
///////////////////////////////////////////////////////////////////////////////////
//// Global Variable
////
LPWSTR g_driverName = L"Microsoft Mirror Driver";
LPVOID g_pVideoMemory;	
HANDLE g_hMapFile;
HANDLE g_hFile;

ULONG  g_init = FALSE;
CScreenCapApp theApp;


BOOL bCallbacked = FALSE;
BITMAPINFOHEADER bmif;
BYTE* pData; 

typedef HRESULT(STDAPICALLTYPE *DwmIsCompositionEnabledFunc)(BOOL* enabled);
typedef HRESULT(STDAPICALLTYPE *DwmEnableCompositionFunc)(UINT action);

///////////////////////////////////////////////////////////////////////////////////
//// Implementation
////
void SetDesktopWindowManager()
{

	static DwmIsCompositionEnabledFunc IsCompositionEnabled = NULL;
	static DwmEnableCompositionFunc		EnableComposition = NULL;
	static BOOL m_bGlass;
	if (IsCompositionEnabled == NULL || EnableComposition == NULL)
	{
		HMODULE dwmModule = LoadLibrary(L"dwmapi.dll");
		if (dwmModule)
		{
			IsCompositionEnabled = reinterpret_cast<DwmIsCompositionEnabledFunc>(GetProcAddress(dwmModule, "DwmIsCompositionEnabled"));
			EnableComposition = reinterpret_cast<DwmEnableCompositionFunc>(GetProcAddress(dwmModule, "DwmEnableComposition"));
		}
	}
	if (IsCompositionEnabled&&EnableComposition)
	{
		IsCompositionEnabled(&m_bGlass);
		if (m_bGlass)
		{
			EnableComposition(0);
		}

	}
}

//--------------------------------------------------------------------------------------
ReleaseError ReleaseResFromDLL(
	CString path, 
	ULONG id
)
{	
	HRSRC	  hSrc			= NULL;
	HANDLE    hFile			= NULL;
	HRSRC     hRsrc			= NULL;
	HGLOBAL   hGlobal		= NULL; 
	HINSTANCE hInst			= NULL;
	DWORD     dwResSize		= 0; 
	DWORD     dwBytesWrite	= 0; 
	DWORD     dwBytesRead	= 0; 
			  
	PVOID     pRsrc			 = NULL;
	PVOID     pConfigEncrypt = NULL;
	 

	hInst = AfxGetInstanceHandle();
	if (!hInst)
	{
		return INVALID_DLL_PATH;
	}

	hRsrc     = FindResource(hInst, MAKEINTRESOURCE(id), L"DRIVER");

	if (!hRsrc)
	{
		return INVALID_RES_NAME;
	}

	dwResSize = SizeofResource(hInst, hRsrc); 
	hGlobal   = LoadResource(hInst, hRsrc);

	if (hGlobal == NULL)
	{ 
		return CANNOT_LOAD_RES;
	}

	pRsrc = LockResource(hGlobal);
	if (!pRsrc)
	{
		return CANNOT_LOCK_RES;
	}

	hFile = CreateFile(path, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return CANNOT_CREATE_FILE;
	}

	if (WriteFile(hFile, pRsrc, dwResSize, &dwBytesWrite, NULL) == FALSE)
	{
		return CANNOT_WRITE_FAILE;
	}

	if (hFile)
	{
		CloseHandle(hFile);
	}
	return EVERYTHING_ALRIGHT;
}
// CScreenCapApp construction

//--------------------------------------------------------------------------------------
CScreenCapApp::CScreenCapApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance

	 
}

//--------------------------------------------------------------------------------------
void GetPrimaryDevice(
	DEVMODE* mode
)
{
	UNREFERENCED_PARAMETER(mode);

	DEVMODE devmode;
	INT   devNum = 0;
	BOOL  result;
	DISPLAY_DEVICE  dispDevice; 
	FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);

	dispDevice.cb = sizeof(DISPLAY_DEVICE);

	// First enumerate for Primary display device:
	while ((result = EnumDisplayDevices(NULL, devNum, &dispDevice, 0)) == TRUE)
	{
		if (dispDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
		{
			// Primary device. Find out its dmPelsWidht and dmPelsHeight.
			EnumDisplaySettings(dispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devmode);
			break;
		}
		devNum++;
	}
}


//--------------------------------------------------------------------------------------
void InitDEVMODE(
	DEVMODE * mode
)
{ 
	FillMemory(mode, sizeof(DEVMODE), 0);
	mode->dmSize = sizeof(DEVMODE);
	mode->dmDriverExtra = 0; 
}


//--------------------------------------------------------------------------------------
BOOL GetMirrorDevice(
	DISPLAY_DEVICE* dispDevice
)
{
	INT   devNum = 0;
	BOOL  result;
	devNum = 0;
	while ((result = EnumDisplayDevices(NULL, devNum, dispDevice, 0)) == TRUE)
	{
		OutputDebugString(&dispDevice->DeviceString[0]);
		if (_wcsicmp(&dispDevice->DeviceString[0], g_driverName) == 0)
			break;
		devNum++;
	} 
	return result;
}

//--------------------------------------------------------------------------------------
BOOL EnableMirror(int dwAttach)
{
	DEVMODE 		devmode1 = { 0 };
	BOOL			change = FALSE;
	DISPLAY_DEVICE dispDevice = { 0 };
	BOOL			ret = false;

	do
	{
		InitDEVMODE(&devmode1);

		// Make sure we have a display on this thread.
		change = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode1);

		devmode1.dmFields = COMMON_MODE;
		devmode1.dmDeviceName[0] = '\0';

		dispDevice.cb = sizeof(DISPLAY_DEVICEA);

		if (change)
		{
			INT code;

			GetPrimaryDevice(&devmode1);

			//Find Mirror driver and Get back the device info   
			if (!(ret = GetMirrorDevice(&dispDevice)))
			{
				ret = FALSE;
				break;
			}

			// Attach and detach information is sent via the dmPelsWidth/Height
			// of the devmode.
			//
			if (dwAttach == 0)
			{
				devmode1.dmPelsWidth = 0;
				devmode1.dmPelsHeight = 0;
			}

			// Update the mirror device's registry data with the devmode. Dont  do a mode change. 
			code = ChangeDisplaySettingsEx(&dispDevice.DeviceName[0], &devmode1, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);

			//Real Mode
			code = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
		}
	} while (FALSE); 

	return ret;
}

//--------------------------------------------------------------------------------------
BOOL InstallInf()
{
	WCHAR			SrcPath[MAX_PATH]; 
	WIN32_FIND_DATA FindFileData;
	CString			m_InfFile = L"";

	//////////////////////////////////////////////////////////
	//	打开INF文件,获取CLASS名和ClassGuid
	 
	CString		m_name;			// name of file
	CString		m_classname;	// name of device class
	HINF		m_hinf;			// open handle
	GUID		m_guid;			// class guid for this INF


	//	从INF文件中装载指定的modal
	CString devid = L"Microsoft_Mirror_Sample1"; 

	//	创建一个DeviceInfoList
	HDEVINFO		m_hinfo;		// handle of device info set
	HWND			m_hwnd = NULL;		// parent window of dialogs
	SP_DEVINFO_DATA	m_devinfo;		// information element for one device 
	CString			m_devid;		// id for the device we're installing
	
	GetCurrentDirectory(MAX_PATH,SrcPath); 
	m_InfFile.Format(L"%ws\\mirror.inf", SrcPath); 
	m_name += m_InfFile;

	OutputDebugString(L"Installing INF... \r\n");
	OutputDebugString(m_InfFile);

	//	确认INF是否存在
	if (FindFirstFile(m_InfFile, &FindFileData) == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(L"FindFirstFile FAILED "); 
		return FALSE;
	} 

	m_hinf = SetupOpenInfFile(m_name, NULL, INF_STYLE_WIN4, NULL); // 打开INF文件
	if (m_hinf == INVALID_HANDLE_VALUE)
	{  
		OutputDebugString(L"INVALID_HANDLE_VALUE\r\n");
		return FALSE;
	}
	//	获取CLASS和ClassGuid
	TCHAR classname[64];
	if (!SetupDiGetINFClass(m_name, &m_guid, classname, arraysize(classname), NULL))
	{
		OutputDebugString(L"SetupDiGetINFClass\r\n");
		return FALSE;
	}
	m_classname = classname;

	DWORD junk;
	//	若INF文件中未指明ClassGuid,则由CLASS名查找ClassGuid.
	//根据系统设备类型的名称获得其的GUID
	if (m_guid == GUID_NULL)
	{ 
		SetupDiClassGuidsFromName(classname, &m_guid, 1, &junk); 
	}
	 
	m_hinfo = SetupDiCreateDeviceInfoList(&m_guid, m_hwnd);//创建设备信息块列表
	if (m_hinfo == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(L"NULL SetupDiCreateDeviceInfoList \r\n");
		return FALSE;
	}
	//	创建一个DeviceInfo
	//	在注册表中创建一个hardware key并设置制定的hardware id
	memset(&m_devinfo, 0, sizeof(m_devinfo));
	m_devinfo.cbSize = sizeof(m_devinfo);

	if (!SetupDiCreateDeviceInfo(m_hinfo, m_classname, &m_guid, NULL, m_hwnd, DICD_GENERATE_ID, &m_devinfo))
	{
		OutputDebugString(L"SetupDiCreateDeviceInfo\r\n");
		return FALSE;//创建设备信息块
	}

	m_devid = devid;

	DWORD size = (devid.GetLength() + 2) * sizeof(TCHAR);
	
	PBYTE hwid = new BYTE[size];
	memset(hwid, 0, size);
	memcpy(hwid, (LPCTSTR)devid, size - 2 * sizeof(TCHAR));
	 
	if (!SetupDiSetDeviceRegistryProperty(m_hinfo, &m_devinfo, SPDRP_HARDWAREID, hwid, size))// 将新的配置值写入
	{
		OutputDebugString(L"SetupDiSetDeviceRegistryProperty FAIL\r\n ");
		return FALSE;
	}

	delete[] hwid; hwid = NULL;

	//If mirror driver doesn't installed
	if (!EnableMirror(1))
	{ 
		SetLastError(0);
		if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, m_hinfo, &m_devinfo))
		{ 
			OutputDebugString(L"SetupDiCallClassInstaller FAIL \r\n");
			return FALSE;
		}  
	}  
	BOOL reboot = FALSE;

	// 下面再搜索那些未连接的设备，更新他们的驱动
	if (!UpdateDriverForPlugAndPlayDevices(m_hwnd, m_devid, m_name, INSTALLFLAG_FORCE, &reboot))
	{  
		AfxMessageBox(L"Update Driver Error \r\n");
		return FALSE;
	}	

	AfxMessageBox(L"Success \r\n");
	
	return TRUE; 
}

//--------------------------------------------------------------------------------------
VOID CreateMapping()
{
	EnableMirror(1);

	g_pVideoMemory = 0;

	CString ptr = L"C:\\video.dat";

	g_hFile = CreateFile(ptr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (g_hFile && g_hFile != INVALID_HANDLE_VALUE)
	{
		g_hMapFile = CreateFileMapping(g_hFile, NULL, PAGE_READWRITE, 0, 0, NULL);

		if (g_hMapFile && g_hMapFile != INVALID_HANDLE_VALUE)
		{
			g_pVideoMemory = MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, 0);

			CloseHandle(g_hMapFile);
		} 
		CloseHandle(g_hFile);
	}
}

//--------------------------------------------------------------------------------------
VOID CreateBitMapHeader(BITMAPFILEHEADER* bmfHdr, LONG width, LONG height)
{ 
	bmfHdr->bfType = BITMAP_MAGIC; // "BM" 
	bmfHdr->bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 4;
	bmfHdr->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
}

//--------------------------------------------------------------------------------------
VOID CreateBitInfoHeader(BITMAPINFOHEADER* binfo, LONG width, LONG height, WORD biBitCount = 32, ULONG biCompression = BI_RGB)
{
	binfo->biSize = sizeof(BITMAPINFOHEADER);
	binfo->biBitCount = biBitCount;
	binfo->biHeight = 0 - height;
	binfo->biWidth = width;
	binfo->biCompression = BI_RGB;
	binfo->biPlanes = 1;
}


//----------------------------------------------------------------------------------------------------------
BOOL SaveAsBitmapFile(BITMAPFILEHEADER* bmfHdr, BITMAPINFOHEADER* binfo_hdr, LONG width, LONG height)
{
	HANDLE hFile;
	DWORD dwBytesWrite  = 0; 
	CString szFileName = L"C:\\CopyScreen2.bmp";

	hFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	// write the BITMAPFILEHEADER 
	if (!WriteFile(hFile, bmfHdr, sizeof(BITMAPFILEHEADER), &dwBytesWrite, NULL))
	{
		szFileName.Format(L"1: %x", GetLastError());
		AfxMessageBox(szFileName);
		return FALSE;
	}

	// write the BITMAPINFOHEADER 
	if (!WriteFile(hFile, binfo_hdr, sizeof(BITMAPINFOHEADER), &dwBytesWrite, NULL))
	{
		szFileName.Format(L"2: %x", GetLastError());
		AfxMessageBox(szFileName);
		return FALSE;
	}
	// write the bmp data 
	if (!WriteFile(hFile, g_pVideoMemory, width * height * 4, &dwBytesWrite, NULL))
	{
		szFileName.Format(L"3: %x", GetLastError());
		AfxMessageBox(szFileName);
		return FALSE;
	}

	if (hFile)
	{
		CloseHandle(hFile);
	}
	return TRUE;
}



//----------------------------------------------------------------------------------------------------------
void SaveFileBMP()
{ 
 
	BITMAPFILEHEADER bmfHdr;
	BITMAPINFOHEADER binfo_hdr;
 
	int width, height; 
	 
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);


	RtlZeroMemory(&bmfHdr, sizeof(BITMAPFILEHEADER));
	CreateBitMapHeader(&bmfHdr, width, height);

	RtlZeroMemory(&binfo_hdr, sizeof(BITMAPINFOHEADER));
	CreateBitInfoHeader(&binfo_hdr, width, height);
	 
	if (SaveAsBitmapFile(&bmfHdr, &binfo_hdr, width, height))
	{
		AfxMessageBox(L"保存成功");
	}
	else
	{
		AfxMessageBox(L"保存失敗");
	}
}


//----------------------------------------------------------------------------------------------------------
BOOL CScreenCapApp::InitInstance()
{
	CWinApp::InitInstance();  
	WCHAR path[256];
	CString str;
	ULONG   count = 0;
	GetCurrentDirectoryW(256, path);
	str.Format(L"%s\\mirror.dll", path);
	if (ReleaseResFromDLL(str, IDR_MIRRORDLL) == EVERYTHING_ALRIGHT)
	{
		OutputDebugString(str);
		count++;
	}

	str.Format(L"%s\\mirror.sys", path);
	if (ReleaseResFromDLL(str, IDR_MIRRORDRV) == EVERYTHING_ALRIGHT)
	{
		OutputDebugString(str);
		count++;
	}
	str.Format(L"%s\\mirror.inf", path);
	if (ReleaseResFromDLL(str, IDR_MIRRORINF) == EVERYTHING_ALRIGHT)
	{
		OutputDebugString(str);
		count++;
	}
	str.Format(L"%s\\mirror.reg", path);
	if (ReleaseResFromDLL(str, IDR_MIRRORREG) == EVERYTHING_ALRIGHT)
	{
		OutputDebugString(str);
		count++;
	}
	return TRUE;
}

//----------------------------------------------------------------------------------------------------------
ULONG WINAPI Init()
{
	InstallInf();
	OutputDebugString(L"installing inf...");

	CreateMapping();
	OutputDebugString(L"creating mapping...");

	return 0;
}

//----------------------------------------------------------------------------------------------------------
PVOID WINAPI ThreadProc(LPVOID params)
{

	OutputDebugString(L"createing bmp...");

	//Wait drawn
	Sleep(3000);

	SaveFileBMP();

	OutputDebugString(L"Done...");
	return 0;
}

//----------------------------------------------------------------------------------------------------------
ULONG WINAPI CopyScreen2()
{
	if (!g_init)
	{
		SetDesktopWindowManager();
		Init();
		g_init = TRUE;
	}
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc, 0, 0, 0); 
	return 0;
}
 
BOOL MagImageScaling(HWND hwnd, void *srcdata, MAGIMAGEHEADER srcheader,
	void *destdata, MAGIMAGEHEADER destheader,
	RECT unclipped, RECT clipped, HRGN dirty) 
{

	// Setup the bitmap info header
	bmif.biSize = sizeof(BITMAPINFOHEADER);
	bmif.biHeight    = srcheader.height;
	bmif.biWidth     = srcheader.width;
	bmif.biSizeImage = srcheader.cbSize;
	bmif.biPlanes	 = 1;
	bmif.biBitCount = (WORD)(bmif.biSizeImage / bmif.biHeight / bmif.biWidth * 8);
	bmif.biCompression = BI_RGB;

	// Prepare the buffer
	if (pData != NULL)
	{
		delete pData;
		pData = NULL;
	}
	pData = (BYTE*)malloc(bmif.biSizeImage);
	memcpy(pData, srcdata, bmif.biSizeImage);

	// The data bit is in top->bottom order, so we convert it to bottom->top order
	LONG lineSize = bmif.biWidth * bmif.biBitCount / 8;
	BYTE* pLineData = new BYTE[lineSize];
	BYTE* pStart;
	BYTE* pEnd;
	LONG lineStart = 0;
	LONG lineEnd = bmif.biHeight - 1;

	while (lineStart < lineEnd)
	{
		// Get the address of the swap line
		pStart = pData + (lineStart * lineSize);
		pEnd = pData + (lineEnd * lineSize);
		// Swap the top with the bottom
		memcpy(pLineData, pStart, lineSize);
		memcpy(pStart, pEnd, lineSize);
		memcpy(pEnd, pLineData, lineSize);

		// Adjust the line index
		lineStart++;
		lineEnd--;
	}
	delete pLineData;
	// Set the flag to say that the callback function is finished
	bCallbacked = TRUE;
	 
	return TRUE;
}

//------------------------------------------------------------------------------------------
void SaveBmpToFile(BITMAPINFOHEADER bmif, BYTE *pData, CString fileName)
{
	// File open
	CFile pFile;
	if (!pFile.Open((LPCTSTR)fileName, CFile::modeCreate | CFile::modeWrite))
	{
		return;
	}

	// Setup the bitmap file header
	BITMAPFILEHEADER bmfh;
	LONG offBits = sizeof(BITMAPFILEHEADER) + bmif.biSize;
	bmfh.bfType = 0x4d42; // "BM"
	bmfh.bfOffBits = offBits;
	bmfh.bfSize = offBits + bmif.biSizeImage;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;

	//Write data to file
	pFile.Write(&bmfh, sizeof(BITMAPFILEHEADER)); // bitmap file header
	pFile.Write(&bmif, sizeof(BITMAPINFOHEADER)); // bitmap info header
	pFile.Write(pData, bmif.biSizeImage); // converted bitmap data

										  // File close
	pFile.Close();
}
 
//----------------------------------------------------------------------------------------------------------
ULONG WINAPI CopyScreen1()
{
	// Get screen resolution
	RECT rect;
	HWND hDesktop = ::GetDesktopWindow();
	ULONG screenX = 0;
	ULONG screenY = 0;
	::GetWindowRect(hDesktop, &rect); 
	screenX = rect.right;
	screenY = rect.bottom;
	  
	dlg = new HostDialog();
	dlg->Create();   
	// Init magnification
	if (!MagInitialize())
	{
		AfxMessageBox(L"err 1");
		return FALSE;
	} 
	HWND hwndMag = CreateWindowA(
			WC_MAGNIFIERA, 
		   "MagnifierWindow",
			WS_CHILD | WS_VISIBLE,
			0,       // default horizontal position 
			0,       // default vertical position 
			screenX,       // default width 
			screenY,       // default height 
			(HWND)dlg->GetSafeHwnd() ,         // no owner window 
			(HMENU)NULL,        // use class menu 
			AfxGetInstanceHandle(),           // handle to application instance 
			(LPVOID)NULL);      // no window-creation data 

	if (hwndMag == NULL)
	{
		AfxMessageBox(L"err 2");
		return FALSE;
	}

	// Set the callback function
	if (!MagSetImageScalingCallback(hwndMag, (MagImageScalingCallback)MagImageScaling))
	{

		AfxMessageBox(L"err 3");
		return FALSE;
	} 
	 
 
	RECT sourceRect;
	sourceRect.top = 0;
	sourceRect.left = 0;
	sourceRect.right = screenX;
	sourceRect.bottom = screenY;

	// Prepare the flag for the callback function
	bCallbacked = FALSE;

	// Set window source to capture entire the desktop
	if (!MagSetWindowSource(hwndMag, sourceRect))
	{

		AfxMessageBox(L"err 3");
		return FALSE;
	}
	 
	while (1)
	{
		if (bCallbacked)
		{
			break;
		}
	} 
	
	SaveBmpToFile(bmif, pData, L"C:\\CopyScreen1.bmp");
	  
	return 0;
}