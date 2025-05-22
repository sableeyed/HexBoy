#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include "hexboy.h"
#include "resource.h"

#define ID_OPEN 1
#define ID_EXIT 2

HFONT font = NULL;
const char CLASS_NAME[] = "HexBoy";
DWORD fileSize = 0;
int scrollPos = 0;
unsigned char *fileData = NULL;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // console window for debugging
    // if(AllocConsole()) {
    //     FILE *dummy;
    //     freopen_s(&dummy, "CONOUT$", "w", stdout);
    //     freopen_s(&dummy, "CONOUT$", "w", stderr);
    // }

    WNDCLASSEX wc = createWindowClass(hInstance);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "HexBoy", WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    if(!hwnd) { return 1; }

    createMenuBar(hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

WNDCLASSEX createWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassEx(&wc);
    
    font = CreateFont(
        -16, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        "Courier New"
    );

    return wc;
}

void createMenuBar(HWND hwnd) {
    HMENU menuBar = CreateMenu();
    HMENU fileMenu = CreateMenu();
    HMENU editMenu = CreateMenu();
    HMENU helpMenu = CreateMenu();
    AppendMenu(fileMenu, MF_STRING, ID_OPEN, "Open");
    AppendMenu(fileMenu, MF_STRING, ID_EXIT, "Exit");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)fileMenu, "File");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)editMenu, "Edit");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)helpMenu, "Help");
    SetMenu(hwnd, menuBar);
}

void openFile(HWND hwnd) {
    char filePath[MAX_PATH] = {0};

    OPENFILENAME file = {0};
    file.lStructSize = sizeof(file);
    file.hwndOwner = hwnd;
    file.lpstrFile = filePath;
    file.nMaxFile = MAX_PATH;
    file.lpstrFilter = "All Files\0*.*\0";
    file.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    file.lpstrTitle = "Open File";

    if(GetOpenFileName(&file)) {
        if(fileData) {
            GlobalFree(fileData);
            fileData = NULL;
            fileSize = 0;
            scrollPos = 0;
        }

        HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(hFile != INVALID_HANDLE_VALUE) {
            fileSize = GetFileSize(hFile, NULL);
            fileData = (unsigned char*) GlobalAlloc(GPTR, fileSize);
            if(fileData) {
                DWORD bytesRead;
                ReadFile(hFile, fileData, fileSize, &bytesRead, NULL);
            }
            CloseHandle(hFile);

            SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE };
            si.nMin = 0;
            si.nMax = (fileSize + 15) / 16 - 1;
            si.nPage = 25;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        }
        scrollPos = 0;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        updateScrollBar(hwnd, 16);
    }
}

void scrollWindow(HWND hwnd, WPARAM wParam) {
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    GetScrollInfo(hwnd, SB_VERT, &si);
    int pos = si.nPos;

    switch (LOWORD(wParam)) {
        case SB_LINEUP: si.nPos -= 1; break;
        case SB_LINEDOWN: si.nPos += 1; break;
        case SB_PAGEUP: si.nPos -= si.nPage; break;
        case SB_PAGEDOWN: si.nPos += si.nPage; break;
        case SB_THUMBTRACK: si.nPos = HIWORD(wParam); break;
    }

    si.fMask = SIF_POS;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    GetScrollInfo(hwnd, SB_VERT, &si);
    if (si.nPos != pos) {
        scrollPos = si.nPos;
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void drawWindow(HWND hwnd, int lineHeight) {
	const char *header = "Offset(h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Decoded Text";
	const char *noFile = "No file loaded";
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	TEXTMETRIC tm;
	RECT client;
	GetClientRect(hwnd, &client);
	
	HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    FillRect(memDC, &client, (HBRUSH)(COLOR_WINDOW + 1));
    SelectObject(memDC, font);
    SetBkMode(memDC, TRANSPARENT);

    GetTextMetrics(memDC, &tm);
    lineHeight = tm.tmHeight;

    int x = 10;
    int y = 0;

    if(fileData != NULL && fileSize > 0) {
    	SetTextColor(memDC, RGB(0, 0, 255));
    	TextOut(memDC, x, y, header, strlen(header));

    	int headerHeight = lineHeight + 4;
        y += headerHeight;

    	int lineStart = scrollPos;
    	int linesPerPage = (client.bottom - headerHeight) / lineHeight;
    	int lineEnd = lineStart + linesPerPage;

    	for(int i = lineStart; i < lineEnd; i++) {
    		int offset = i * 16;
    		
    		if((DWORD)offset >= fileSize) { 
    			break;
    		}

    		int drawX = x;

    		char offsetStr[12];
    		sprintf(offsetStr, "%08X", offset);
    		SetTextColor(memDC, RGB(0, 0, 255));
    		TextOut(memDC, drawX, y, offsetStr, strlen(offsetStr));
    		drawX += 10 * tm.tmAveCharWidth;

    		for(int z = 0; z < 16; z++) {
    			char hexByte[4] = "   ";
    			if((DWORD)(offset + z) < fileSize) {
    				sprintf(hexByte, "%02X ", fileData[offset + z]);
    			}
    			SetTextColor(memDC, (z % 2 == 0) ? RGB(0, 0, 0) : RGB(100, 100, 100));
    			TextOut(memDC, drawX, y, hexByte, 3);
    			drawX += 3 * tm.tmAveCharWidth;
    		}

    		drawX += tm.tmAveCharWidth;

    		SetTextColor(memDC, RGB(0, 0, 0));
    		for(int z = 0; z < 16; z++) {
    			char asciiChar = ' ';
    			if((DWORD)(offset + z) < fileSize) {
    				unsigned char c = fileData[offset + z];
    				asciiChar = c >= 32 ? c : '.';
    			}
    			TextOut(memDC, drawX, y, &asciiChar, 1);
    			drawX += tm.tmAveCharWidth;
    		}

    		y += lineHeight;
    	}
    }
    else {
    	FillRect(memDC, &client, GetSysColorBrush(COLOR_BTNFACE));
        SetTextColor(memDC, RGB(80, 80, 80));
        SIZE msgSize;
        GetTextExtentPoint32(memDC, noFile, strlen(noFile), &msgSize);
        int cx = (client.right - msgSize.cx) / 2;
        int cy = (client.bottom - msgSize.cy) / 2;
        TextOut(memDC, cx, cy, noFile, strlen(noFile));
    }

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

void updateScrollBar(HWND hwnd, int lineHeight) {
    RECT client;
    GetClientRect(hwnd, &client);

    int headerHeight = lineHeight + 4;

    int totalLines = (fileSize + 15) / 16;
    int visibleLines = (client.bottom - headerHeight) / lineHeight;
    SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS };
    si.nMin = 0;
    si.nMax = totalLines - 1;
    si.nPage = visibleLines;
    if(scrollPos > (int)(si.nMax - si.nPage + 1)) {
        scrollPos = max(0, si.nMax - si.nPage + 1);
    }
    si.nPos = scrollPos;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int lineHeight = 16;
    switch (uMsg) {
        case WM_COMMAND: //called when a menu item is clicked
            switch (LOWORD(wParam)) {
                case ID_OPEN:
                    openFile(hwnd);
                    break;
                case ID_EXIT:
                    PostQuitMessage(0);
                    //FreeConsole();
                    break;
            }
            return 0;
        case WM_PAINT: //called when the window needs to be redrawn
            drawWindow(hwnd, lineHeight);
            return 0;
        case WM_VSCROLL: //called when the user scrolls the scroll bar
            scrollWindow(hwnd, wParam);
            return 0;
        case WM_MOUSEWHEEL:
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int lines = delta / WHEEL_DELTA;
            for(int x = 0; x < abs(lines); x++) {
                WPARAM wScroll = (lines > 0) ? SB_LINEUP : SB_LINEDOWN;
                scrollWindow(hwnd, wScroll);
            }
            return 0;
        case WM_CREATE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            //FreeConsole();
            return 0;
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, TRUE);
            drawWindow(hwnd, lineHeight);
            updateScrollBar(hwnd, lineHeight);
            break;
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}