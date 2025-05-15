#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include "hexboy.h"

#define ID_OPEN 1
#define ID_EXIT 2

HFONT font = NULL;
const char CLASS_NAME[] = "HexBoy";
DWORD fileSize = 0;
int scrollPos = 0;
unsigned char *fileData = NULL;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // console window for debugging
    if(AllocConsole()) {
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }

    WNDCLASS wc = createWindowClass(hInstance);

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

WNDCLASS createWindowClass(HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    
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
        InvalidateRect(hwnd, NULL, TRUE);
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
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    //SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    SelectObject(hdc, font);
    
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    lineHeight = tm.tmHeight;
    
    int lineStart = scrollPos;
    int linesPerPage = (ps.rcPaint.bottom - ps.rcPaint.top) / lineHeight;
    int lineEnd = lineStart + linesPerPage;
    
    char line[128];
    int x = 10, y = 0;
    for (int i = lineStart; i < lineEnd; i++) {
        int offset = i * 16;
        if ((DWORD)offset >= fileSize) break;
        
        int len = sprintf(line, "%08X  ", offset);
        for (int j = 0; j < 16; j++) {
            if ((DWORD)(offset + j) < fileSize)
            len += sprintf(line + len, "%02X ", fileData[offset + j]);
            else
            len += sprintf(line + len, "   ");
        }
        
        len += sprintf(line + len, " ");
        for (int j = 0; j < 16; j++) {
            if ((DWORD)(offset + j) < fileSize) {
                unsigned char c = fileData[offset + j];
                //line[len++] = (c >= 32 && c <= 126) ? c : '.';
                line[len++] = c >= 32 ? c : '.';
            } else {
                line[len++] = ' ';
            }
        }
        line[len] = '\0';
        TextOut(hdc, x, y, line, len);
        y += lineHeight;
    }
    
    EndPaint(hwnd, &ps);
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
                    FreeConsole();
                    break;
            }
            return 0;
        case WM_PAINT: //called when the window needs to be redrawn
            drawWindow(hwnd, lineHeight);
            return 0;
        case WM_VSCROLL: //called when the user scrolls the scroll bar
            scrollWindow(hwnd, wParam);
            return 0;
        case WM_CREATE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            FreeConsole();
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}