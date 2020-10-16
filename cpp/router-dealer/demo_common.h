#ifndef _ROUTER_DEALER_DEMO_COMMON_H_
#define _ROUTER_DEALER_DEMO_COMMON_H_

#include <string>

static const std::string gNewConnMsg("MsgConnectStart");
static const std::string gDisConnMsg("MsgConnectEnd");

static const std::string gBindAddr("tcp://0.0.0.0:11000");
static const std::string gConnectAddr("tcp://127.0.0.1:11000");
//static const std::string gBindAddr("ipc:///tmp/zmq_router_server");
//static const std::string gConnectAddr("ipc:///tmp/zmq_router_server");

static const std::string gDealerServerId("NodeServer");
static const std::string gDealerClientId("NodeClient");

#endif //_ROUTER_DEALER_COMMON_H_
