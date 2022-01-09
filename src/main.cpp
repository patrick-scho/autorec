#ifndef WIN32
  #define WIN32
#endif
#include <WinSock2.h>
#include "win.h"
#include "ws.h"

#define LAY_IMPLEMENTATION
#include "layout.h"

#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shlwapi.h>

void
startRecording()
{
  ws::sendRequest("StartRecord");
}

void
stopRecording()
{
  ws::sendRequest("StopRecord");
}

bool
checkProcessRunning(HANDLE handle)
{
  DWORD exit_code;

  if (handle != NULL && GetExitCodeProcess(handle, &exit_code)) {
    return exit_code == STILL_ACTIVE;
  }

  return false;
}

HANDLE
getHwndProcess(HWND hwnd)
{
  DWORD processId, threadId = GetWindowThreadProcessId(hwnd, &processId);
  return OpenProcess(PROCESS_QUERY_INFORMATION, false, processId);
}

// HWND getProcessHwnd(HANDLE handle) {}

bool
checkFullscreenWindow()
{
  HWND desktopHwnd = GetDesktopWindow();
  HWND fgHwnd = GetForegroundWindow();

  if (fgHwnd != desktopHwnd && fgHwnd != GetShellWindow()) {
    RECT windowRect, desktopRect;
    // Get Window and Desktop size
    GetWindowRect(fgHwnd, &windowRect);
    GetWindowRect(desktopHwnd, &desktopRect);

    bool fullscreen = windowRect.bottom == desktopRect.bottom &&
                      windowRect.top == desktopRect.top &&
                      windowRect.left == desktopRect.left &&
                      windowRect.right == desktopRect.right;

    return fullscreen;
  }

  return false;
}

bool
checkForegroundProcess(std::string exeName)
{
  HWND fgHwnd = GetForegroundWindow();
  HANDLE fgHandle = getHwndProcess(fgHwnd);

  char filename[1024];
  int len = GetModuleFileNameExA(fgHandle, NULL, filename, 1024);
  PathStripPathA(filename);

  return strcmp(filename, exeName.c_str()) == 0;
}

bool recording = false;
HANDLE process = NULL;

int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{

  HWND window = win::Window("Title", "MyWindowClass", hInstance);

  lay_id row1 = lay_item(&win::_::ctx);
  lay_insert(&win::_::ctx, win::_::root, row1);
  lay_set_size_xy(&win::_::ctx, row1, 0, 25);
  lay_set_behave(&win::_::ctx, row1, LAY_LEFT);
  lay_set_contain(&win::_::ctx, row1, LAY_ROW);
  lay_set_margins_ltrb(&win::_::ctx, row1, 5, 5, 5, 5);
  lay_id row2 = lay_item(&win::_::ctx);
  lay_insert(&win::_::ctx, win::_::root, row2);
  lay_set_size_xy(&win::_::ctx, row2, 0, 0);
  lay_set_behave(&win::_::ctx, row2, LAY_FILL);
  lay_set_contain(&win::_::ctx, row2, LAY_ROW);
  lay_set_margins_ltrb(&win::_::ctx, row2, 5, 5, 5, 5);
  lay_id col1 = lay_item(&win::_::ctx);
  lay_set_size_xy(&win::_::ctx, col1, 80, 0);
  lay_set_behave(&win::_::ctx, col1, LAY_VCENTER);
  lay_set_contain(&win::_::ctx, col1, LAY_COLUMN);
  lay_set_margins_ltrb(&win::_::ctx, col1, 5, 0, 5, 0);

  HWND cbWindowTitle = win::CheckBox(window, "Window Title", row1, 100, 25, 0, 0);
  HWND cbFullscreenWindow = win::CheckBox(window, "Any Fullscreen Application", row1, 200, 25, 0, 0);

  HWND btnConnect = win::Button(window, "Connect", row1, 100, 25, 0, 0);
  win::Callback(btnConnect, BN_CLICKED, [&]() {
    ws::connect("ws://127.0.0.1:4444");
  });

  win::Callback(cbWindowTitle, BN_CLICKED, [&]() {
    SendMessageA(cbWindowTitle, BM_SETCHECK, SendMessageA(cbWindowTitle, BM_GETCHECK, 0, 0) ? BST_UNCHECKED : BST_CHECKED, 0);
  });
  win::Callback(cbFullscreenWindow, BN_CLICKED, [&]() {
    SendMessageA(cbFullscreenWindow, BM_SETCHECK, SendMessageA(cbFullscreenWindow, BM_GETCHECK, 0, 0) ? BST_UNCHECKED : BST_CHECKED, 0);
  });

  HWND lstActiveProcesses = win::ListView(window, row2, 0, 0, 0, LAY_FILL);
  lay_insert(&win::_::ctx, row2, col1);
  HWND lstMonitoredProcesses = win::ListBox(window, row2, 0, 0, 0, LAY_FILL);
  win::AddStyle(lstActiveProcesses, WS_VSCROLL);
  win::AddStyle(lstMonitoredProcesses, WS_VSCROLL);

  HWND btnUpdateWindows = win::Button(window, "Update", col1, 85, 25, 0, 0);
  HWND btnStartMonitoringName = win::Button(window, "Exe name >>", col1, 85, 25, 0, 0);
  HWND btnStartMonitoringPath = win::Button(window, "Full path >>", col1, 85, 25, 0, 0);
  HWND btnStopMonitoring = win::Button(window, "Remove", col1, 85, 25, 0, 0);
  win::Callback(btnUpdateWindows, BN_CLICKED, [&]() {
    win::ListClear(lstActiveProcesses);
    for (HWND hwnd = GetTopWindow(NULL); hwnd != nullptr;
         hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
      if (!IsWindowVisible(hwnd))
        continue;

      RECT rect;
      GetWindowRect(hwnd, &rect);

      char str[1024];
      if (GetModuleFileNameExA(getHwndProcess(hwnd), 0, str, 1024) != 0 &&
          win::ListFindString(lstActiveProcesses, str) == LB_ERR) {
        win::ListAddString(lstActiveProcesses, str);
      }
    }
  });
  win::Callback(btnStartMonitoringName, BN_CLICKED, [&]() {
    int sel = win::ListGetSelectedIndex(lstActiveProcesses);
    if (sel < 0) return;

    std::string selStr = win::ListGetText(lstActiveProcesses, sel);
    
    char *filename = new char[selStr.size()];
    std::memcpy(filename, selStr.c_str(), selStr.size());
    PathStripPathA(filename);

    if (win::ListFindString(lstMonitoredProcesses, std::string(filename)) == LB_ERR)
      win::ListAddString(lstMonitoredProcesses, std::string(filename));

    delete[] filename;
  });
  win::Callback(btnStartMonitoringPath, BN_CLICKED, [&]() {
    int sel = win::ListGetSelectedIndex(lstActiveProcesses);
    if (sel < 0) return;
    std::string selStr = win::ListGetText(lstActiveProcesses, sel);
    if (win::ListFindString(lstMonitoredProcesses, selStr) == LB_ERR)
    win::ListAddString(lstMonitoredProcesses, selStr);
  });
  win::Callback(btnStopMonitoring, BN_CLICKED, [&]() {
    int sel = win::ListGetSelectedIndex(lstMonitoredProcesses);
    if (sel < 0) return;
    win::ListRemove(lstMonitoredProcesses, sel);
  });

  win::ShowWindow(window);

  ws::init();

  SetTimer(window, 10123, 100, [](HWND, UINT, UINT_PTR, DWORD) {
    if (!recording) {
      if (checkForegroundProcess("League of Legends.exe")) {
        recording = true;
        process = getHwndProcess(GetForegroundWindow());
        startRecording();
      }
    } else {
      if (!checkProcessRunning(process)) {
        recording = false;
        process = NULL;
        stopRecording();
      }
    }
  });

  while (win::UpdateWindow(window)) {
    ws::update();
  }
}
