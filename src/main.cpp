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

// int WINAPI
// WinMain(HINSTANCE hInstance,
//         HINSTANCE hPrevInstance,
//         LPSTR lpCmdLine,
//         int nCmdShow)
int main(int argc, char **argv)
{

  //win::Window window("Title", "MyWindowClass", hInstance);
  win::Window window("Title", "MyWindowClass", GetModuleHandle(0));

  lay_context *ctx = &window.ctx;
  lay_id root = window.lId;

  win::Hwnd row1(&window, &window, 0, 0, 0, 25, LAY_ROW, LAY_LEFT);
  lay_set_margins_ltrb(ctx, row1.lId, 5, 5, 5, 5);
  win::Hwnd row2(&window, &window, 0, 0, 0, 0, LAY_ROW, LAY_FILL);
  lay_set_margins_ltrb(ctx, row2.lId, 5, 5, 5, 5);

  win::CheckBox cbWindowTitle(&window, &row1, "Window Title", 100, 25, 0, 0);
  win::CheckBox cbFullscreenWindow(&window, &row1, "Any Fullscreen Application", 200, 25, 0, 0);

  win::Button btnConnect(&window, &row1, "Connect", 100, 25, 0, 0);
  btnConnect.onClick([&]() {
    ws::connect("ws://127.0.0.1:4444");
  });

  win::ListBox lstActiveProcesses(&window, &row2, 0, 0, 0, LAY_FILL);
  
  win::Hwnd col1(&window, &row2, 0, 0, 80, 0, LAY_COLUMN, LAY_VCENTER);
  lay_set_margins_ltrb(ctx, col1.lId, 5, 0, 5, 0);

  win::ListBox lstMonitoredProcesses(&window, &row2, 0, 0, 0, LAY_FILL);
  lstActiveProcesses.addStyle(WS_VSCROLL);
  lstMonitoredProcesses.addStyle(WS_VSCROLL);

  win::Button btnUpdateWindows(&window, &col1, "Update", 85, 25, 0, 0);
  win::Button btnStartMonitoringName(&window, &col1, "Exe name >>", 85, 25, 0, 0);
  win::Button btnStartMonitoringPath(&window, &col1, "Full path >>", 85, 25, 0, 0);
  win::Button btnStopMonitoring(&window, &col1, "Remove", 85, 25, 0, 0);
  btnUpdateWindows.onClick([&]() {
    lstActiveProcesses.clear();
    for (HWND hwnd = GetTopWindow(NULL); hwnd != nullptr;
         hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
      if (!IsWindowVisible(hwnd))
        continue;

      RECT rect;
      GetWindowRect(hwnd, &rect);

      char str[1024];
      if (GetModuleFileNameExA(getHwndProcess(hwnd), 0, str, 1024) != 0 &&
          lstActiveProcesses.findString(str) == LB_ERR) {
        lstActiveProcesses.addString(str);
      }
    }
  });
  btnStartMonitoringName.onClick([&]() {
    int sel = lstActiveProcesses.getSelectedIndex();
    if (sel < 0) return;

    std::string selStr = lstActiveProcesses.getText(sel);
    
    char *filename = new char[selStr.size()];
    std::memcpy(filename, selStr.c_str(), selStr.size());
    PathStripPathA(filename);

    if (lstMonitoredProcesses.findString(std::string(filename)) == LB_ERR)
      lstMonitoredProcesses.addString(std::string(filename));

    delete[] filename;
  });
  btnStartMonitoringPath.onClick([&]() {
    int sel = lstActiveProcesses.getSelectedIndex();
    if (sel < 0) return;
    std::string selStr = lstActiveProcesses.getText(sel);
    if (lstMonitoredProcesses.findString(selStr) == LB_ERR)
    lstMonitoredProcesses.addString(selStr);
  });
  btnStopMonitoring.onClick([&]() {
    int sel = lstMonitoredProcesses.getSelectedIndex();
    if (sel < 0) return;
    lstMonitoredProcesses.remove(sel);
  });

  window.show();

  ws::init();

  SetTimer(window.hwnd, 10123, 100, [](HWND, UINT, UINT_PTR, DWORD) {
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

  while (window.update()) {
    ws::update();
  }
}
