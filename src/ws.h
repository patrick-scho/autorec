#include "mongoose.h"

#include "json.hpp"
using json = nlohmann::json;

#include <string>
#include <functional>

namespace ws
{
  mg_mgr mgr;
  mg_connection *c = nullptr;

  bool isConnected = false;

  std::function<void()> onConnect;
  std::function<void()> onClose;
  std::function<void()> onError;

  static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_WS_OPEN) {
      puts("Open");
      isConnected = true;
    } else if (ev == MG_EV_WS_MSG) {
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
      printf("Msg: %.*s\n", (int)wm->data.len, wm->data.ptr);
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
        puts("Connected");
        if (onConnect)
          onConnect();
      }
    }

    if (ev == MG_EV_ERROR) {
      printf("Error: %s\n", (char*)ev_data);
      isConnected = false;
      if (onError)
          onError();
    }
    if (ev == MG_EV_CLOSE) {
      puts("Close");
      isConnected = false;
      if (onClose)
        onClose();
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
    if (c)
      mg_mgr_poll(&mgr, 10);
  }

  void sendRequest(std::string requestType)
  {
    json request = { { "op", 6 },
                      { "d",
                        {
                          { "requestType", requestType },
                          { "requestId", std::to_string(time(nullptr)).c_str() }
                        } } };
    auto requestStr = request.dump();
    mg_ws_send(c, requestStr.c_str(), requestStr.size(), WEBSOCKET_OP_TEXT);
  }
}