#ifndef WIN32
  #define WIN32
#endif
#include <WinSock2.h>
#include "win.h"
#include "ws.h"

#define LAY_IMPLEMENTATION
#include "layout.h"

#include <fstream>
#include <vector>
#include <string>

#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shlwapi.h>

bool recording = false;
HANDLE process = NULL;
HWND hwnd;
HINSTANCE hInstance;
HICON icon_white, icon_green, icon_red;

std::vector<std::string> gameExes;


NOTIFYICONDATAA niData = { 0 };
const UINT ICON_MSG = WM_APP+1;
void
ShowNotificationIcon()
{
  Shell_NotifyIconA(NIM_ADD, &niData);
  Shell_NotifyIconA(NIM_SETVERSION, &niData);
}

void
HideNotificationIcon()
{
  Shell_NotifyIconA(NIM_DELETE, &niData);
}

void changeIcon(HWND hwnd, HICON icon)
{
  niData.hIcon = icon;
  if (! IsWindowVisible(hwnd))
    Shell_NotifyIconA(NIM_MODIFY, &niData);
  SendMessage(hwnd, WM_SETICON, 0, (LPARAM)icon);
  SendMessage(hwnd, WM_SETICON, 1, (LPARAM)icon);
}



void
startRecording()
{
  ws::sendRequest("StartRecord");
  changeIcon(hwnd, icon_green);
}

void
stopRecording()
{
  ws::sendRequest("StopRecord");
  changeIcon(hwnd, icon_red);
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

  char filename[MAX_PATH];
  int len = GetModuleFileNameExA(fgHandle, NULL, filename, MAX_PATH);

  if (strcmp(filename, exeName.c_str()) == 0)
    return true;

  PathStripPathA(filename);

  return strcmp(filename, exeName.c_str()) == 0;
}

void ReadGameExes()
{
  std::ifstream ifs("games.txt");

  gameExes.clear();

  while (ifs) {
    std::string str;
    std::getline(ifs, str);
    if (! str.empty())
      gameExes.push_back(str);
  }

  ifs.close();
}

void WriteGameExes()
{
  std::ofstream ofs("games.txt", std::ios::trunc);

  for (const auto &exe : gameExes)
  {
    ofs.write(exe.c_str(), exe.size());
    ofs.write("\n", 1);
  }

  ofs.close();
}

/*
TODO:
  - Disconnect while recording
*/



int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
//int main(int argc, char **argv)
{
  hInstance = GetModuleHandle(0);

  icon_white = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON_WHITE));
  icon_green = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON_GREEN));
  icon_red = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON_RED));

  win::Window window("Title", "MyWindowClass", hInstance);
  hwnd = window.hwnd;

  niData.cbSize = sizeof(niData);
  niData.uID = 12345;
  niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  niData.hIcon = icon_white;
  niData.hWnd = window.hwnd;
  niData.uCallbackMessage = ICON_MSG;
  niData.uVersion = NOTIFYICON_VERSION_4;

  window.handlers[WM_SIZE].push_back([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (wParam == SIZE_MINIMIZED) {
      ShowNotificationIcon();
      ShowWindow(hwnd, false);
    }
  });
  window.handlers[WM_DESTROY].push_back([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HideNotificationIcon();
  });  
  window.handlers[ICON_MSG].push_back([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (LOWORD(lParam) == NIN_SELECT) {
      HideNotificationIcon();
      ShowWindow(hwnd, true);
      SetForegroundWindow(hwnd);
      SetActiveWindow(hwnd);
    }
  });

  window.handlers[WM_GETMINMAXINFO].push_back([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
          MINMAXINFO *mmInfo = (MINMAXINFO*)lParam;
          mmInfo->ptMinTrackSize.x = 400;
          mmInfo->ptMinTrackSize.y = 200;
  });

  lay_context *ctx = &window.ctx;
  lay_id root = window.lId;

  lay_id row1 = win::createLayId(&window.ctx, window.lId, 0, 25, LAY_ROW, LAY_LEFT);
  lay_set_margins_ltrb(ctx, row1, 5, 5, 5, 5);
  lay_id row2 = win::createLayId(&window.ctx, window.lId, 0, 0, LAY_ROW, LAY_FILL);
  lay_set_margins_ltrb(ctx, row2, 5, 5, 5, 5);

  win::CheckBox cbWindowTitle(&window, "Window Title", row1, 100, 25, 0, 0);
  win::CheckBox cbFullscreenWindow(&window, "Any Fullscreen Application", row1, 200, 25, 0, 0);

  win::Button btnConnect(&window, "Connect", row1, 100, 25, 0, 0);
  btnConnect.onClick([&]() {
    if (! ws::isConnected)
      ws::connect("ws://127.0.0.1:4455");
  });

  win::ListBox lstActiveProcesses(&window, row2, 0, 0, 0, LAY_FILL);
  
  lay_id col1 = win::createLayId(&window.ctx, row2, 80, 0, LAY_COLUMN, LAY_VCENTER);
  lstActiveProcesses.addStyle(WS_VSCROLL);

  lay_set_margins_ltrb(ctx, col1, 5, 0, 5, 0);

  win::ListBox lstMonitoredProcesses(&window, row2, 0, 0, 0, LAY_FILL);
  lstMonitoredProcesses.addStyle(WS_VSCROLL);

  ReadGameExes();
  for (const auto &exe : gameExes)
    lstMonitoredProcesses.addString(exe);

  win::Button btnUpdateWindows(&window, "Update", col1, 85, 25, 0, 0);
  win::Button btnStartMonitoringName(&window, "Exe name >>", col1, 85, 25, 0, 0);
  win::Button btnStartMonitoringPath(&window, "Full path >>", col1, 85, 25, 0, 0);
  win::Button btnStopMonitoring(&window, "Remove", col1, 85, 25, 0, 0);
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
    
    char *filename = new char[selStr.size()+1];
    std::memcpy(filename, selStr.c_str(), selStr.size());
    filename[selStr.size()] = '\0';
    PathStripPathA(filename);

    std::string filenameStr(filename);

    if (lstMonitoredProcesses.findString(filenameStr) == LB_ERR)
    {
      lstMonitoredProcesses.addString(filenameStr);
      gameExes.push_back(filenameStr);
      WriteGameExes();
    }

    delete[] filename;
  });
  btnStartMonitoringPath.onClick([&]() {
    int sel = lstActiveProcesses.getSelectedIndex();
    if (sel < 0) return;
    std::string selStr = lstActiveProcesses.getText(sel);
    if (lstMonitoredProcesses.findString(selStr) == LB_ERR)
    {
      lstMonitoredProcesses.addString(selStr);
      gameExes.push_back(selStr);
      WriteGameExes();  
    }
  });
  btnStopMonitoring.onClick([&]() {
    int sel = lstMonitoredProcesses.getSelectedIndex();
    if (sel < 0) return;
    lstMonitoredProcesses.remove(sel);
    gameExes.erase(gameExes.begin() + sel);
    WriteGameExes();
  });

  window.show();
  window.setDefaultFont();

  ws::onConnect = [&]() {
    changeIcon(window.hwnd, icon_red);
    btnConnect.setActive(false);
  };
  ws::onClose = [&]() {
    changeIcon(window.hwnd, icon_white);
    btnConnect.setActive(true);
  };
  ws::onError = ws::onClose;
  ws::init();

  window.setTimer(1000, [&btnConnect]() {
    if (! ws::isConnected)
      ws::connect("ws://127.0.0.1:4455");
  });

  window.setTimer(100, []() {
    if (!recording) {
      for (auto exe : gameExes) {
        if (checkForegroundProcess(exe)) {
          recording = true;
          process = getHwndProcess(GetForegroundWindow());
          startRecording();
        } 
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

  return 0;
}
