#include "mongoose.h"

#include "json.hpp"
using json = nlohmann::json;

#include <string>

namespace ws
{
  mg_mgr mgr;
  mg_connection *c = nullptr;

  bool done = false;

  static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_WS_OPEN) {
    } else if (ev == MG_EV_WS_MSG) {
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
      std::string jsonStr(wm->data.ptr, wm->data.len);
      auto msg = json::parse(jsonStr);
      int op = msg["op"].get<int>();
      
      if (op == 0) {
        json response = {
          {"op", 1},
          {"d", {
            {"rpcVersion", 1},
          }}
        };
        auto responseStr = response.dump();
        mg_ws_send(c, responseStr.c_str(), responseStr.size(), WEBSOCKET_OP_TEXT);
      }
      else if (op == 2)
      {
        MessageBoxA(NULL, "hura", "connected", MB_OK);
      }
    }

    if (ev == MG_EV_ERROR) {
      MessageBoxA(NULL, "", "Error", MB_OK);
      done = true;
    }
    if (ev == MG_EV_CLOSE) {
      MessageBoxA(NULL, "", "Close", MB_OK);
      done = true;
    }
  }

  void init()
  {
    mg_mgr_init(&mgr);
  }

  void connect(std::string address)
  {
    c = mg_ws_connect(&mgr, address.c_str(), cb, nullptr, nullptr);
  }

  void update()
  {
    if (c && !done)
      mg_mgr_poll(&mgr, 10);
  }

  void sendRequest(std::string requestType)
  {
    json request = { { "op", 6 },
                      { "d",
                        {
                          { "requestType", requestType },
                          { "requestId", std::to_string(NULL).c_str() }
                        } } };
    auto requestStr = request.dump();
    mg_ws_send(c, requestStr.c_str(), requestStr.size(), WEBSOCKET_OP_TEXT);
  }
}