#include "dealer_node.h"
#include "demo_common.h"

static bool stopped = false;
static void SignalHandler(int s) {
    std::cout << "Catch signal" << std::endl;
    stopped = true;
}


DealerNode::DealerNode() {
    context_ = std::unique_ptr<zmq::context_t>(new zmq::context_t(2));
    socket_ = std::make_shared<zmq::socket_t>(*context_, ZMQ_DEALER);
}

DealerNode::~DealerNode() {
}

int DealerNode::Init(const std::string& dealer_id) {
    int reconnect_ms = 1000;
    int rcv_buffer = 10;
    int snd_buffer = 10;
    int recv_timeout = 6000;
    int zmq_immediate = 1;
    socket_->setsockopt(ZMQ_RECONNECT_IVL, &reconnect_ms, sizeof(reconnect_ms));
    socket_->setsockopt(ZMQ_RCVHWM, &rcv_buffer, sizeof(rcv_buffer));
    socket_->setsockopt(ZMQ_SNDHWM, &snd_buffer, sizeof(snd_buffer));
    socket_->setsockopt(ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    socket_->setsockopt(ZMQ_IMMEDIATE, &zmq_immediate, sizeof(zmq_immediate));
    socket_->setsockopt(ZMQ_IDENTITY, dealer_id.c_str(), dealer_id.size());
    
    return 0;
}

void DealerNode::Destroy() {
    if(socket_) {
        socket_->close();
        socket_ = nullptr;
    }
    if(context_) {
        context_->close();
        context_ = nullptr;
    }
}

int DealerNode::Connect(const std::string& url) {
    url_ = url;
    socket_->connect(url_);

    {
        // send connect info
        SendMultipart(gNewConnMsg, nullptr, 0);

        // recv response
        zmq::message_t rsp_msg;
        zmq::detail::recv_result_t result = socket_->recv(rsp_msg);
        if (!result.has_value()) {
            std::cout << "[ERROR] " << "recv rsp failed" << std::endl;
            return -1;
        }
        int64_t more;
        size_t more_size = sizeof(more);
        socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
        if(more) {
             std::cout << "[ERROR] " << "dealer socket not recv all msg" << std::endl;
             return -1;
        }

        std::string rsp_str = std::string((char*)rsp_msg.data(), rsp_msg.size());
        // rsp_str == "OK";
    }

    recv_thread_running_ = true;
    recv_thread_.reset(new std::thread(&DealerNode::RecvHandler, this));

    return 0;
}

void DealerNode::DisConnect() {
    recv_thread_running_ = false;
    if (recv_thread_->joinable()) {
        recv_thread_->join();
    }
    std::cout << "recv thread end" << std::endl;
    {
        // send disconnect info
        SendMultipart(gDisConnMsg, nullptr, 0);
    }
    socket_->disconnect(url_);
}

void DealerNode::RecvHandler() {
    std::cout << "RecvHandler" << std::endl;
    while (recv_thread_running_) {
        RecvData msg_info;
        int ret = RecvMultipart(msg_info);
        if(ret < 0) {
            // return recv data error
            {
                std::unique_lock<std::mutex> lock(recv_mutex_);
                recv_condition_.notify_all();
                lock.unlock();
            }
            continue;
        }
        std::unique_lock<std::mutex> lock(recv_mutex_);
        recv_list_.push_back(msg_info);
        recv_condition_.notify_all();
        lock.unlock();
    }

    std::cout << "RecvHandler end" << std::endl;
}


int DealerNode::SendTo(std::string& dst, const char* buf, int size) {
    return SendMultipart(dst, buf, size);
}

int DealerNode::RecvFrom(std::string& src_id, std::vector<char>& buf) {
    std::unique_lock<std::mutex> lock(recv_mutex_);
    if(recv_list_.empty()) {
        recv_condition_.wait(lock);
    }
    if(recv_list_.empty()) {
        std::cout << "recv list empty" << std::endl;
        return 0;
    }
    auto msg_info = recv_list_.front();
    recv_list_.pop_front();
    lock.unlock();

    src_id = msg_info.msg_id;
    buf.resize(msg_info.msg_data->size());
    memcpy(buf.data(), msg_info.msg_data->data(), msg_info.msg_data->size());

    return buf.size();
}

int DealerNode::SendMultipart(const std::string& dst, const char* buf, int size) {
    std::unique_lock<std::mutex> lock(send_multipart_mutex_);
    zmq::message_t dst_id(dst.c_str(), dst.size());
    zmq::detail::send_result_t result = socket_->send(dst_id, zmq::send_flags::sndmore);
    if (!result.has_value()) {
        std::cout << "[ERROR] " << "send id failed" << std::endl;
        return -1;
    }

    zmq::message_t send_msg;
    if(size > 0) {
        send_msg.rebuild(buf, size);
    }
    result = socket_->send(send_msg, zmq::send_flags::none);
    if (!result.has_value()) {
        std::cout << "[ERROR] " << "send data failed" << std::endl;
        return -1;
    }

    return result.value();
}

int DealerNode::RecvMultipart(RecvData& msg_info) {
    std::unique_lock<std::mutex> lock(recv_multipart_mutex_);
    int err = 0;
    int msg_count = 0;

    while(recv_thread_running_) {
        zmq::detail::recv_result_t result = socket_->recv(*msg_info.msg_data.get());
        if (!result.has_value()) {
            std::cout << "[ERROR] " << "recv msg failed, msg count:" << msg_count << std::endl;
            err = -1;
        } else {
            switch(msg_count) {
                case 0:
                  {
                      msg_info.msg_id = std::string((char*)msg_info.msg_data->data(), msg_info.msg_data->size());
                      break;
                  }
                default:
                  break;
            }
        }

        msg_count++;

        int64_t more;
        size_t more_size = sizeof(more);
        socket_->getsockopt(ZMQ_RCVMORE, &more, &more_size);
        if(!more) {
            break;
        }
    }

    if(err < 0) {
        return err;
    }
    if(msg_count != 2) {
        std::cout << "[ERROR] " << "Recv msg count:" << msg_count << std::endl;
        return -1;
    }

    return msg_info.msg_data->size();
}


