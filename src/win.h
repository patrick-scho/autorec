#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <CommCtrl.h>

#include "layout.h"

#include <functional>
#include <map>
#include <string>

using std::string;

namespace win {
namespace _ {
using CallbackFn = std::function<void()>;
std::map<HWND, std::map<WORD, CallbackFn>> handlers;
std::map<HWND, lay_id> lIds;

NOTIFYICONDATA niData = { 0 };
void
ShowNotificationIcon()
{
  Shell_NotifyIconA(NIM_ADD, &_::niData);
  Shell_NotifyIconA(NIM_SETVERSION, &_::niData);
}

void
HideNotificationIcon()
{
  Shell_NotifyIconA(NIM_DELETE, &_::niData);
}

lay_context ctx;
lay_id root;

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      Shell_NotifyIconA(NIM_DELETE, &niData);
      lay_destroy_context(&ctx);
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      if (wParam == SIZE_MINIMIZED) {
        ShowNotificationIcon();
        ShowWindow(hwnd, false);
      }
      else {
        lay_set_size_xy(&_::ctx, _::root, LOWORD(lParam), HIWORD(lParam));
        lay_run_context(&_::ctx);

        for (auto &lId : lIds) {
          lay_vec4 rect = lay_get_rect(&_::ctx, lId.second);
          SetWindowPos(lId.first, HWND_TOP,
            rect[0],
            rect[1],
            rect[2],
            rect[3],
            SWP_NOZORDER
          );
        }
        RedrawWindow(hwnd, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
      }
      break;
    case WM_COMMAND:
      if (handlers.find((HWND)lParam) != handlers.end()) {
        auto handler = handlers[(HWND)lParam];
        if (handler.find(HIWORD(wParam)) != handler.end()) {
          auto cb = handler[HIWORD(wParam)];
          cb();
        }
      }
      break;
    case WM_NOTIFY:
      break;
    case WM_APP + 1:
      if (LOWORD(lParam) == NIN_SELECT) {
        HideNotificationIcon();
        ShowWindow(hwnd, true);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
      }
      break;
    case WM_CTLCOLORSTATIC:
      return (LONG)GetStockObject(WHITE_BRUSH);
    case WM_GETMINMAXINFO: {
      MINMAXINFO *mmInfo = (MINMAXINFO*)lParam;
      mmInfo->ptMinTrackSize.x = 400;
      mmInfo->ptMinTrackSize.y = 200;
      break;
    }
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}
}


void
Callback(HWND hwnd, WORD ev, std::function<void()> cb)
{
  _::handlers[hwnd][ev] = cb;
}

HWND
Window(string title, string className, HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = _::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = className.c_str();
  wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
  RegisterClassEx(&wc);

  lay_init_context(&_::ctx);
  _::root = lay_item(&_::ctx);
  lay_set_contain(&_::ctx, _::root, LAY_COLUMN);

  HWND result = CreateWindowA(className.c_str(),
                       title.c_str(),
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       nullptr,
                       nullptr,
                       hInstance,
                       nullptr);
                       
  _::niData.cbSize = sizeof(_::niData);
  _::niData.uID = 12345;
  _::niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  _::niData.hIcon = LoadIconA(nullptr, IDI_WINLOGO);
  _::niData.hWnd = result;
  _::niData.uCallbackMessage = WM_APP+1;
  _::niData.uVersion = NOTIFYICON_VERSION_4;

  return result;
}

bool
UpdateWindow(HWND hwnd)
{
  MSG msg;
  if (GetMessage(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return true;
  }
  return false;
}

void
ShowWindow(HWND hwnd)
{
  ShowWindow(hwnd, true);

  EnumChildWindows(
    hwnd,
    [](HWND hwnd, LPARAM lParam) -> BOOL {
      HFONT guiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
      SendMessage(hwnd, WM_SETFONT, (WPARAM)guiFont, MAKELPARAM(TRUE, 0));
      return TRUE;
    },
    0);
}

HWND
Button(HWND hwnd, string title, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
{
  lay_id lId = lay_item(&_::ctx);
  lay_insert(&_::ctx, parent, lId);
  lay_set_size_xy(&_::ctx, lId, w, h);
  lay_set_contain(&_::ctx, lId, contain);
  lay_set_behave(&_::ctx, lId, behave);

  HWND result = CreateWindowExA(0,
                                WC_BUTTONA,
                                title.c_str(),
                                WS_VISIBLE | WS_CHILD,
                                0, 0, 0, 0,
                                hwnd,
                                nullptr,
                                nullptr,
                                nullptr);
  _::lIds[result] = lId;
  return result;
}

HWND
ListBox(HWND hwnd, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
{
  lay_id lId = lay_item(&_::ctx);
  lay_insert(&_::ctx, parent, lId);
  lay_set_size_xy(&_::ctx, lId, w, h);
  lay_set_contain(&_::ctx, lId, contain);
  lay_set_behave(&_::ctx, lId, behave);

  HWND result = CreateWindowExA(0,
                         WC_LISTBOXA,
                         "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         0, 0, 0, 0,
                         hwnd,
                         nullptr,
                         nullptr,
                         nullptr);
  _::lIds[result] = lId;
  return result;
}

void
ListAddString(HWND hwnd, string str)
{
  SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)str.c_str());
}

int
ListGetSelectedIndex(HWND hwnd)
{
  int sel = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
  return sel;
}

int ListFindString(HWND hwnd, string str)
{
  return SendMessageA(hwnd, LB_FINDSTRINGEXACT, -1, (LPARAM)str.c_str());
}

string
ListGetText(HWND hwnd, int index)
{
  char buffer[1024];
  SendMessage(hwnd, LB_GETTEXT, index, (LPARAM)buffer);
  return string(buffer);
}

void
ListClear(HWND hwnd)
{
  SendMessageA(hwnd, LB_RESETCONTENT, 0, 0);
}

void ListRemove(HWND hwnd, int index)
{
  SendMessageA(hwnd, LB_DELETESTRING, index, 0);
}

HWND
ListView(HWND hwnd, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
{
  lay_id lId = lay_item(&_::ctx);
  lay_insert(&_::ctx, parent, lId);
  lay_set_size_xy(&_::ctx, lId, w, h);
  lay_set_contain(&_::ctx, lId, contain);
  lay_set_behave(&_::ctx, lId, behave);

  HWND result = CreateWindowExA(0,
                         WC_LISTVIEWA,
                         "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         0, 0, 0, 0,
                         hwnd,
                         nullptr,
                         nullptr,
                         nullptr);
  _::lIds[result] = lId;
  return result;
}

HWND
CheckBox(HWND hwnd, string title, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
{
  lay_id lId = lay_item(&_::ctx);
  lay_insert(&_::ctx, parent, lId);
  lay_set_size_xy(&_::ctx, lId, w, h);
  lay_set_contain(&_::ctx, lId, contain);
  lay_set_behave(&_::ctx, lId, behave);

  HWND result = CreateWindowExA(0,
                         WC_BUTTONA,
                         title.c_str(),
                         WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                         0, 0, 0, 0,
                         hwnd,
                         nullptr,
                         nullptr,
                         nullptr);
  _::lIds[result] = lId;
  return result;
}

void SetStyle(HWND hwnd, DWORD style)
{
  SetWindowLongPtrA(hwnd, GWL_STYLE, style);
}
DWORD GetStyle(HWND hwnd)
{
  return GetWindowLongPtrA(hwnd, GWL_STYLE);
}
void AddStyle(HWND hwnd, DWORD style)
{
  SetWindowLongPtrA(hwnd, GWL_STYLE, GetStyle(hwnd) | style);
}
void RemoveStyle(HWND hwnd, DWORD style)
{
  SetWindowLongPtrA(hwnd, GWL_STYLE, GetStyle(hwnd) & (~style));
}
}