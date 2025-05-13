#include <windows.h>
#include <commdlg.h>
#include <stdio.h>

#define ID_OPEN 1
#define ID_EXIT 2

unsigned char* fileData = NULL;
DWORD fileSize = 0;
int scrollPos = 0;
HFONT font = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "HexBoy";

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

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "HexBoy", WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_OPEN, "Open");
    AppendMenu(hFileMenu, MF_STRING, ID_EXIT, "Exit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hFileMenu, "File");
    SetMenu(hwnd, hMenuBar);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (fileData) GlobalFree(fileData);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int lineHeight = 16;

    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_OPEN: {
                    char filePath[MAX_PATH] = {0};

                    OPENFILENAME ofn = {0};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = "All Files\0*.*\0";
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    ofn.lpstrTitle = "Open File";

                    if (GetOpenFileName(&ofn)) {
                        if (fileData) {
                            GlobalFree(fileData);
                            fileData = NULL;
                            fileSize = 0;
                            scrollPos = 0;
                        }

                        HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            fileSize = GetFileSize(hFile, NULL);
                            fileData = (unsigned char*) GlobalAlloc(GPTR, fileSize);
                            if (fileData) {
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
                    break;
                }
                case ID_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;

        case WM_VSCROLL: {
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
            return 0;
        }

        case WM_PAINT: {
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
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}