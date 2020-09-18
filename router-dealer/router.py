import zmq
from threading import Thread


class RouterServer(object):
    def __init__(self):
        ctx = zmq.Context.instance()
        self.socket_router = ctx.socket(zmq.ROUTER)
        self.socket_router.bind("tcp://*:12000")
        self.socket_router.setsockopt(zmq.RCVHWM, 100)

        self.clients = {}
        self.router_id = b'router'

    def ConnectRsp(self, conn_addr):
        all_clients = b''
        for c in self.clients:
            all_clients = all_clients + c + b';'
        self.socket_router.send_multipart([conn_addr, all_clients])
        self.clients[conn_addr] = 1

    def RouterThread(self):
        while True:
            # 发送端地址、接收端地址、发送的数据  ps: 此处的send_addr, recv_addr, send_data均是bytes类型
            send_addr, recv_addr, send_data = self.socket_router.recv_multipart()
            print((send_addr, recv_addr, send_data))
            # 把发送端存入workers

            if len(recv_addr) == 0 and send_data == b'New Connect':
                # recv addr为空, 表示新连接, 注册并返回连接成功响应
                self.ConnectRsp(send_addr)
                continue

            if send_addr not in self.clients:
                print("send addr not register, ignore it")
                continue

            if recv_addr == b'broadcast':
                for ident in self.clients:
                    self.socket_router.send_multipart([ident, send_addr, send_data])
                continue

            if recv_addr == b'heartbeat':
                # --TODO: 心跳超时, 删除self.clients中的send_addr
                continue

            if recv_addr in self.clients:
                print("send msg from %r to %r" % (send_addr, recv_addr))
                # 如果接收端地址存在,把返回的send_data转发给接收端
                self.socket_router.send_multipart([recv_addr, send_addr, send_data])
            else:
                # 接收端不存在
                print("dst ident %r not exist, send from %r" % (recv_addr, send_addr))
            #print(self.clients)

    def Start(self):
        print("Start Server")
        Thread(target=self.RouterThread).start()

    def Send(self, recv_ident, msg):
        self.socket_router.send_multipart([recv_ident, self.router_id, msg])

if __name__ == '__main__':
    s = RouterServer()
    s.Start()
    while True:
        send_ident = bytes(input("send_ident:"), encoding='utf8')
        send_msg = bytes(input("send_msg:"), encoding='utf8')
        s.Send(send_ident, send_msg)
