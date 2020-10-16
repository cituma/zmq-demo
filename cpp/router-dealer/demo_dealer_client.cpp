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

    int ret = dealer->Init(gDealerClientId);
    if(ret) {
        std::cout << "Dealer init err:" << ret << std::endl;
        return 0;
    }
    ret = dealer->Connect(gConnectAddr);
    if(ret) {
        std::cout << "Dealer connect err:" << ret << std::endl;
        return 0;
    }

    while(!stopped) {
        std::string src_id;
        std::vector<char> recv_buf;
        int ret = dealer->RecvFrom(src_id, recv_buf);
        if(ret < 0) {
            std::cout << "[ERROR] " << "recv data error:" << ret << std::endl;
        } else {
            std::string recv_str(&recv_buf[0], recv_buf.size());
            std::cout << "Recv from:" << src_id << ". msg:" << recv_str << std::endl;
        }
    }


    dealer->DisConnect();
    dealer->Destroy();

    return 0;
}
