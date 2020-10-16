#ifndef _DEALER_DEMO_NODE_H_
#define _DEALER_DEMO_NODE_H_

#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <list>
#include <condition_variable>
#include <unordered_set>
#include <zmq_addon.hpp>

class DealerNode {
public:
    DealerNode();
    ~DealerNode();
    int Init(const std::string& dealer_id);
    void Destroy();
    int Connect(const std::string& url);
    void DisConnect();
    int SendTo(std::string& dst, const char* buf, int size);
    int RecvFrom(std::string& src, std::vector<char>& buf);

private:
    class RecvData {
    public:
        RecvData() {
          msg_data = std::make_shared<zmq::message_t>();
        }
        ~RecvData() {
          msg_data = nullptr;
        }
        std::string  msg_id;
        std::shared_ptr<zmq::message_t> msg_data;
    };

private:
    void RecvHandler();
    int SendMultipart(const std::string& dst, const char* buf, int size);
    int RecvMultipart(RecvData& msg_info);

private:
    std::unique_ptr<zmq::context_t> context_;
    std::shared_ptr<zmq::socket_t>  socket_;
    std::string                     url_;

    // recv thread
    bool recv_thread_running_;
    std::unique_ptr<std::thread> recv_thread_;
    std::mutex recv_mutex_;
    std::condition_variable recv_condition_;

    std::mutex              send_multipart_mutex_;
    std::mutex              recv_multipart_mutex_;

    std::list<RecvData>             recv_list_;
};


#endif //_DEALER_DEMO_NODE_H_

