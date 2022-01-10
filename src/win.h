#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <CommCtrl.h>

#include "../res/resource.h"

#include "layout.h"

#include <functional>
#include <unordered_map>
#include <string>


namespace win
{
  struct Window;
  struct Hwnd
  {
    HWND hwnd;
    lay_id lId;
    Window *window;

    Hwnd() {}
    Hwnd(Window *window, LPCSTR className, LPCSTR windowName, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave);

    void setStyle(DWORD style)
    {
      SetWindowLongPtrA(hwnd, GWL_STYLE, style);
    }
    DWORD getStyle()
    {
      return GetWindowLongPtrA(hwnd, GWL_STYLE);
    }
    void addStyle(DWORD style)
    {
      SetWindowLongPtrA(hwnd, GWL_STYLE, getStyle() | style);
    }
    void removeStyle(DWORD style)
    {
      SetWindowLongPtrA(hwnd, GWL_STYLE, getStyle() & (~style));
    }
  };
  struct Window : Hwnd
  {
  private:

    static LRESULT CALLBACK
    WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
      Window *window = (Window*)GetWindowLongPtrA(hwnd, 0);
      if (window == nullptr)
        return DefWindowProc(hwnd, msg, wParam, lParam);

      bool defaultHandler = false;

      switch (msg) {
        case WM_CLOSE:
          DestroyWindow(hwnd);
          break;
        case WM_DESTROY:
          lay_destroy_context(&window->ctx);
          PostQuitMessage(0);
          break;
        case WM_SIZE:
          if (wParam != SIZE_MINIMIZED) {
            lay_set_size_xy(&window->ctx, window->lId, LOWORD(lParam), HIWORD(lParam));
            lay_run_context(&window->ctx);

            //RedrawWindow(hwnd, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
          }
          break;
        case WM_NOTIFY:
          break;
        case WM_CTLCOLORSTATIC:
          return (LONG_PTR)GetStockObject(WHITE_BRUSH);
        default:
          defaultHandler = true;
          break;
      }
      
      for (auto handler : window->handlers[msg])
        handler(hwnd, msg, wParam, lParam);

      if (defaultHandler)
        return DefWindowProc(hwnd, msg, wParam, lParam);
      else
        return 0;
    }
  public:
    lay_context ctx;
    std::unordered_map<UINT,
      std::vector<
        std::function<void(HWND, UINT, WPARAM, LPARAM)>>> handlers;

    Window(std::string title, std::string className, HINSTANCE hInstance)
    {
      WNDCLASSEXA wc;
      wc.cbSize = sizeof(WNDCLASSEX);
      wc.style = 0;
      wc.lpfnWndProc = WndProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = sizeof(Window*);
      wc.hInstance = hInstance;
      wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_WHITE));
      wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
      wc.lpszMenuName = nullptr;
      wc.lpszClassName = className.c_str();
      wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_WHITE));
      RegisterClassExA(&wc);

      lay_init_context(&ctx);
      lId = lay_item(&ctx);
      lay_set_contain(&ctx, lId, LAY_COLUMN);

      hwnd = CreateWindowA(className.c_str(),
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

      SetWindowLongPtrA(hwnd, 0, (LONG_PTR)this);
    }
    bool update()
    {
      MSG msg;
      if (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return true;
      }
      return false;
    }
    void show()
    {
      ShowWindow(hwnd, true);
    }
    void setDefaultFont()
    {
      EnumChildWindows(
        hwnd,
        [](HWND hwnd, LPARAM lParam) -> BOOL {
          HFONT guiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
          SendMessage(hwnd, WM_SETFONT, (WPARAM)guiFont, MAKELPARAM(TRUE, 0));
          return TRUE;
        },
        0);
    }
  };

  struct Button : Hwnd
  {
    Button(Window *window, std::string title, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
      : Hwnd(window, WC_BUTTONA, title.c_str(), parent, w, h, contain, behave)
    {
      window->handlers[WM_COMMAND].push_back([&](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if ((HWND)lParam == this->hwnd && HIWORD(wParam) == BN_CLICKED)
          for (auto handler : this->onClickHandlers)
            handler();
      });
    }

    void onClick(std::function<void()> cb)
    {
      onClickHandlers.push_back(cb);
    }
  private:
    std::vector<std::function<void()>> onClickHandlers;
  };
  
  struct CheckBox : Hwnd
  {
    CheckBox(Window *window, std::string title, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
      : Hwnd(window, WC_BUTTONA, title.c_str(), parent, w, h, contain, behave)
    {
      addStyle(BS_CHECKBOX);
      window->handlers[WM_COMMAND].push_back([&](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if ((HWND)lParam == this->hwnd && HIWORD(wParam) == BN_CLICKED)
          SendMessageA(this->hwnd, BM_SETCHECK, SendMessageA(this->hwnd, BM_GETCHECK, 0, 0) ? BST_UNCHECKED : BST_CHECKED, 0);
      });
    }
  };

  struct ListBox : Hwnd
  {
    ListBox(Window *window, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
      : Hwnd(window, WC_LISTBOXA, "", parent, w, h, contain, behave)
    {
      addStyle(WS_BORDER);
      //addStyle(WS_VSCROLL);
    }

    void addString(std::string str)
    {
      SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }

    int getSelectedIndex()
    {
      return SendMessage(hwnd, LB_GETCURSEL, 0, 0);
    }

    int findString(std::string str)
    {
      return SendMessageA(hwnd, LB_FINDSTRINGEXACT, -1, (LPARAM)str.c_str());
    }

    std::string getText(int index)
    {
      int len = SendMessageA(hwnd, LB_GETTEXTLEN, index, 0);
      std::string result(len, 0);
      SendMessage(hwnd, LB_GETTEXT, index, (LPARAM)result.data());
      return result;
    }

    void clear()
    {
      SendMessageA(hwnd, LB_RESETCONTENT, 0, 0);
    }

    void remove(int index)
    {
      SendMessageA(hwnd, LB_DELETESTRING, index, 0);
    }
  };

  lay_id createLayId(lay_context *ctx, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
  {
    lay_id lId = lay_item(ctx);
    lay_insert(ctx, parent, lId);
    lay_set_size_xy(ctx, lId, w, h);
    lay_set_contain(ctx, lId, contain);
    lay_set_behave(ctx, lId, behave);
    return lId;
  }
}




win::Hwnd::Hwnd(Window *window, LPCSTR className, LPCSTR windowName, lay_id parent, lay_scalar w, lay_scalar h, uint32_t contain, uint32_t behave)
{
  this->window = window;
  
  lId = createLayId(&window->ctx, parent, w, h, contain, behave);
  
  hwnd = CreateWindowExA(0,
                          className,
                          windowName,
                          WS_VISIBLE | WS_CHILD,
                          0,
                          0,
                          0,
                          0,
                          window->hwnd,
                          nullptr,
                          nullptr,
                          nullptr);

  window->handlers[WM_SIZE].push_back([this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    lay_vec4 rect = lay_get_rect(&this->window->ctx, this->lId);
    SetWindowPos(this->hwnd, HWND_TOP,
      rect[0],
      rect[1],
      rect[2],
      rect[3],
      SWP_NOZORDER
    );
  });
}