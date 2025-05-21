#ifndef HEXBOY_H
#define HEXBOY_H

#include <windows.h>
#include <commdlg.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WNDCLASSEX createWindowClass(HINSTANCE hInstance);
void createMenuBar(HWND hwnd);
void openFile(HWND hwnd);
void scrollWindow(HWND hwnd, WPARAM wParam);
void drawWindow(HWND hwnd, int lineHeight);
void updateScrollBar(HWND hwnd, int lineHeight);

#endif