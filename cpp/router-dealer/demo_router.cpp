#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <zmq_addon.hpp>
#include "demo_common.h"

static bool stopped = false;
static void SignalHandler(int s) {
    std::cout << "Catch signal" << std::endl;
    stopped = true;
}


class DemoRouter {
public:
    DemoRouter();
    ~DemoRouter();
    int Init();
    void Destroy();
    int Start(const std::string& url);
    void Stop();

private:
    void RouterHandler();
    int RecvMultipart(std::string& src_id, std::string& dst_id, zmq::message_t& data_msg);
    int SendMultipart(std::string& dst_id, const char* buf, int size);
    int SendMultipart(std::string& dst_id, std::string& src_id, const char* buf, int size);
    int ConnectRsp(std::string& src_id);
    void DisConnect(std::string& src_id);

private:
    std::unique_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t>  socket_;

    bool router_running_;
    std::unique_ptr<std::thread>    router_thread_;

    std::unordered_set<std::string> dealer_nodes_;
};

DemoRouter::DemoRouter()
    : context_(nullptr)
    , socket_(nullptr)
    , router_running_(false)
{
}

DemoRouter::~DemoRouter() {
    router_running_ = false;
    context_ = nullptr;
    socket_ = nullptr;
}

int DemoRouter::Init() {
    context_ = std::unique_ptr<zmq::context_t>(new zmq::context_t(2));
    socket_ = std::make_shared<zmq::socket_t>(*context_, ZMQ_ROUTER);

    int max_rcv_buf = 10;
    int max_snd_buf = 10;
    socket_->setsockopt(ZMQ_RCVHWM, &max_rcv_buf, sizeof(max_rcv_buf));
    socket_->setsockopt(ZMQ_SNDHWM, &max_snd_buf, sizeof(max_snd_buf));

    return 0;
}

void DemoRouter::Destroy() {
    if(socket_) {
        //socket_->close();
        socket_ = nullptr;
    }
    if(context_) {
        context_->close();
        context_ = nullptr;
    }
}

int DemoRouter::Start(const std::string& url) {
    std::cout << "Start router" << std::endl;
    socket_->bind(url);

    router_running_ = true;
    router_thread_.reset(new std::thread(&DemoRouter::RouterHandler, this));

    return 0;
}

void DemoRouter::Stop() {
    std::cout << "Stop router" << std::endl;
    router_running_ = false;
    socket_->close();
    if (router_thread_->joinable()) {
        router_thread_->join();
    }
}

int DemoRouter::ConnectRsp(std::string& src_id) {
    std::string rsp("OK");
    SendMultipart(src_id, rsp.c_str(), rsp.size());

    std::cout << "New dealer:" << src_id << std::endl;
    dealer_nodes_.insert(src_id);

    return 0;
}

void DemoRouter::DisConnect(std::string& src_id) {
    auto it = dealer_nodes_.find(src_id);
    if(it != dealer_nodes_.end()) {
        dealer_nodes_.erase(it);
    }
}

void DemoRouter::RouterHandler() {
    std::cout << "RouterHandler" << std::endl;
    while(router_running_) {
        std::string src_id, dst_id;
        zmq::message_t data_msg;
        int ret = RecvMultipart(src_id, dst_id, data_msg);
        if(ret < 0) {
            std::cout << "RecvMultipart error:" << ret << std::endl;;
            continue;
        }

        if(dst_id == gNewConnMsg) {
            //new connect
            ConnectRsp(src_id);
            continue;
        } else if(dst_id == gDisConnMsg) {
            //disconnect
            DisConnect(src_id);
            continue;
        }

        if(src_id == dst_id) {
            std::cout << "src id " << src_id << " is equal to dst id, ignore it." << std::endl;
            continue;
        }

        if(dealer_nodes_.find(src_id) == dealer_nodes_.end()) {
            std::cout << "src id " << src_id << " not register, ignore it." << std::endl;
            continue;
        }

        if(dealer_nodes_.find(dst_id) != dealer_nodes_.end()) {
            std::cout << "send msg from " << src_id << " to " << dst_id << std::endl;
            SendMultipart(dst_id, src_id, (const char*)data_msg.data(), data_msg.size());
            continue;
        } 
    }
    std::cout << "RouterHandler end" << std::endl;
}


int DemoRouter::RecvMultipart(std::string& src_id, std::string& dst_id, zmq::message_t& data_msg) {
    int err = 0;
    int msg_count = 0;

    while(router_running_) {
        zmq::detail::recv_result_t result = socket_->recv(data_msg);
        if (!result.has_value()) {
            std::cout << "recv msg failed, msg count:" << msg_count << std::endl;
            err = -1;
        } else {
            switch(msg_count) {
                case 0:
                  {
                      src_id = std::string((char*)data_msg.data(), data_msg.size());
                      break;
                  }
                case 1:
                  {
                      dst_id = std::string((char*)data_msg.data(), data_msg.size());
                      break;
                  }
                default:
                  break;
            }
        }

        msg_count++;

        int64_t more;
        size_t more_size = sizeof (more);
        socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
        if(!more) {
            break;
        }
    }
    if(err < 0) {
        return err;
    }
    if(msg_count != 3) {
        std::string payload = std::string((char*)data_msg.data(), data_msg.size());
        std::cout << "Recv msg count:" << msg_count 
          << ". src:" << src_id << ", dst:" << dst_id
          << ". payload:" << payload << std::endl;
        return -1;
    }

    return data_msg.size();
}

int DemoRouter::SendMultipart(std::string& dst_id, const char* buf, int size) {
    zmq::message_t msg(dst_id.c_str(), dst_id.size());
    socket_->send(msg, zmq::send_flags::sndmore);
    
    zmq::message_t data_msg;
    if(size > 0) {
        data_msg.rebuild(buf, size);
    }
    zmq::detail::send_result_t result = socket_->send(data_msg, zmq::send_flags::none);
    if (!result.has_value()) {
        std::cout << "send data failed" << std::endl;
        return -1;
    }

    return result.value();
}

int DemoRouter::SendMultipart(std::string& dst_id, std::string& src_id, const char* buf, int size) {
    zmq::message_t dst_msg(dst_id.c_str(), dst_id.size());
    socket_->send(dst_msg, zmq::send_flags::sndmore);

    zmq::message_t src_msg(src_id.c_str(), src_id.size());
    socket_->send(src_msg, zmq::send_flags::sndmore);

    zmq::message_t data_msg;
    if(size > 0) {
        data_msg.rebuild(buf, size);
    }
    zmq::detail::send_result_t result = socket_->send(data_msg, zmq::send_flags::none);
    if (!result.has_value()) {
        std::cout << "send data failed" << std::endl;
        return -1;
    }

    return result.value();
}


int main(int argc, char* argv[]) {
    signal(SIGINT, SignalHandler);

    std::shared_ptr<DemoRouter> router = std::make_shared<DemoRouter>();

    int ret = router->Init();
    if(ret) {
        std::cout << "Router init err:" << ret << std::endl;
        return 0;
    }
    ret = router->Start(gBindAddr);
    if(ret) {
        std::cout << "Router start err:" << ret << std::endl;
        return 0;
    }

    int sleep_ms = 10;
    while(!stopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    router->Stop();
    router->Destroy();

    router = nullptr;
    return 0;
}
