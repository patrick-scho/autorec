#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <CommCtrl.h>

HWND hwndButton1;
HWND hwndList1;

const char g_szClassName[] = "MyWindowClass";

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED) {
        if ((HWND)lParam == hwndButton1) {
          SendMessage(hwndList1, LB_ADDSTRING, 0, (LPARAM)"Hello");
        }
      }
      break;
    case WM_NOTIFY:
      break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
  WNDCLASSEX wc;
  HWND hwnd;
  MSG Msg;

  // Step 1: Registering the Window Class
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = g_szClassName;
  wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);


  if (!RegisterClassEx(&wc)) {
    MessageBox(nullptr,
               "Window Registration Failed!",
               "Error!",
               MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  // Step 2: Creating the Window
  hwnd = CreateWindowA(g_szClassName,
                        "The title of my window",
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        nullptr,
                        nullptr,
                        hInstance,
                        nullptr);

  HWND tab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 10, 10, 300, 300, hwnd, nullptr, nullptr, nullptr);
  hwndButton1 = CreateWindowExA(0, WC_BUTTONA, "OK", WS_VISIBLE | WS_CHILD, 10, 10, 100, 100, tab, nullptr, nullptr, nullptr);
  hwndList1   = CreateWindowExA(0, WC_LISTBOXA, "OK", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL, 110, 10, 100, 100, tab, nullptr, nullptr, nullptr);

  TCITEMA tie;
  tie.mask = TCIF_TEXT;
  tie.pszText = "Hallo";
  TabCtrl_InsertItem(tab, 0, &tie);
  tie.mask = TCIF_TEXT;
  tie.pszText = "Ciao";
  TabCtrl_InsertItem(tab, 1, &tie);

  if (hwnd == nullptr) {
    MessageBox(
      nullptr, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);
  
  UpdateWindow(hwnd);

  EnumChildWindows(hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
    HFONT guiFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwnd, WM_SETFONT, (WPARAM)guiFont, MAKELPARAM(TRUE, 0));
    return TRUE;
  }, 0);

  // Step 3: The Message Loop
  while (GetMessage(&Msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }
  return Msg.wParam;
}