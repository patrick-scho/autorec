// #define _WEBSOCKETPP_CPP11_INTERNAL_
// #define ASIO_STANDALONE
// #include <websocketpp/config/asio_no_tls_client.hpp>
// #include <websocketpp/client.hpp>

// using client = websocketpp::client<websocketpp::config::asio_client>;

#include "mongoose.h"

#include "json.hpp"
using json = nlohmann::json;

#include <string>

namespace ws
{
  // client c;

  // void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
  //   auto j = json::parse(msg->get_payload());
  //   int op = j["op"].get<int>();

  //   if (op == 0) {
  //     json response = { { "op", 1 },
  //                       { "d", {
  //                           { "rpcVersion", 1 },
  //                         } } };
  //     auto responseStr = response.dump();
  //     websocketpp::lib::error_code ec;
  //     c.send(hdl, responseStr, websocketpp::frame::opcode::TEXT, ec);
  //     if (ec)
  //       MessageBoxA(NULL, ec.message().c_str(), "error", MB_OK);
  //   } else if (op == 2) {
  //     MessageBoxA(NULL, "hura", "connected", MB_OK);
  //   }
  // }

  // void init()
  // {
  //   c.set_access_channels(websocketpp::log::alevel::all);
  //   c.clear_access_channels(websocketpp::log::alevel::frame_payload);
  //   c.set_error_channels(websocketpp::log::elevel::all);

  //   // Initialize ASIO
  //   c.init_asio();

  //   // Register our message handler
  //   c.set_message_handler(&on_message);
  // }

  // void connect(std::string address)
  // {
  //   websocketpp::lib::error_code ec;
  //   client::connection_ptr con = c.get_connection(address, ec);
  //   if (ec) {
  //       std::cout << "could not create connection because: " << ec.message() << std::endl;
  //       return;
  //   }

  //   c.connect(con);
  // }

  // void update()
  // {
  //   c.run_one();
  // }





  mg_mgr mgr;
  mg_connection *c = nullptr;

  bool done = false;

  static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
      MessageBoxA(NULL, "", "Error", MB_OK);
    } else if (ev == MG_EV_WS_OPEN) {
      //mg_ws_send(c, "hello", 5, WEBSOCKET_OP_TEXT);
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
        MessageBoxA(NULL,
          (std::string("is_client") + std::to_string(c->is_client) +
          std::string(" is_accepted") + std::to_string(c->is_accepted) +
          std::string(" is_readable") + std::to_string(c->is_readable) +
          std::string(" is_writable") + std::to_string(c->is_writable)).c_str(),
          "", MB_OK);
        mg_ws_send(c, "{\"op\":1,\"d\":{\"rpcVersion\":1}}", 29, WEBSOCKET_OP_TEXT);
      }
      else if (op == 2)
      {
        MessageBoxA(NULL, "hura", "connected", MB_OK);
      }
    }

    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
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
    else
      MessageBoxA(NULL, "cant update", "Nio", MB_OK);
  }

  void identify()
  {
        MessageBoxA(NULL,
          (std::string("is_client") + std::to_string(c->is_client) +
          std::string(" is_accepted") + std::to_string(c->is_accepted) +
          std::string(" is_readable") + std::to_string(c->is_readable) +
          std::string(" is_writable") + std::to_string(c->is_writable)).c_str(),
          "", MB_OK);
    mg_ws_send(c, "{\"op\":1,\"d\":{\"rpcVersion\":1}}", 29, WEBSOCKET_OP_TEXT);
  }
}