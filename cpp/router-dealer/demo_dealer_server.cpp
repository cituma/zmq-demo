#include <iostream>
#include <signal.h>
#include <vector>
#include <thread>
#include <mutex>
#include <list>
#include <unordered_set>
#include "demo_common.h"
#include "dealer_node.h"

static bool stopped = false;
static void SignalHandler(int s) {
    std::cout << "Catch signal" << std::endl;
    stopped = true;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, SignalHandler);

    std::shared_ptr<DealerNode> dealer = std::make_shared<DealerNode>();

    int ret = dealer->Init(gDealerServerId);
    if(ret) {
        std::cout << "Dealer init err:" << ret << std::endl;
        return 0;
    }
    ret = dealer->Connect(gConnectAddr);
    if(ret) {
        std::cout << "Dealer connect err:" << ret << std::endl;
        return 0;
    }

    int sleep_ms = 500; //500 ms
    int idx = 0;
    std::string dst_id = gDealerClientId;
    while(!stopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

        std::string send_data = "This is a message " + std::to_string(++idx);
        int ret = dealer->SendTo(dst_id, (const char*)send_data.c_str(), send_data.size());
        if(ret < 0) {
            std::cout << "[ERROR] " << "send data error:" << ret << std::endl;
        }
    }

    dealer->DisConnect();
    dealer->Destroy();

    return 0;
}
