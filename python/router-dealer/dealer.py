import zmq
import sys
from threading import Thread

class DealerClient(object):
    def __init__(self, ident):
        ctx = zmq.Context.instance()
        self.socket = ctx.socket(zmq.DEALER)
        self.socket.setsockopt(zmq.IDENTITY, ident)
        #socket.setsockopt(zmq.RCVTIMEO, 1000)
        self.socket.connect("tcp://localhost:12000")
        print("socket connect to router!!!")
        self.socket.send_multipart([b'', b'New Connect'])
        message = self.socket.recv()
        print("response:%r" % message)

    def Send(self, ident, work):
        self.socket.send_multipart([ident, work])

    def RecvThread(self):
        while True:
            try:
                # 获取客户端地址和消息内容
                send_addr, message = self.socket.recv_multipart()
                print("recv from:%r, recv msg:%r" % (send_addr, message))
            except Exception as e:
                print(e)
                continue

    def Recv(self):
        Thread(target=self.RecvThread).start()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("%r local_ident" % sys.argv[0])
        exit(0)

    local_ident = bytes(sys.argv[1], encoding='utf8')

    c = DealerClient(local_ident)
    c.Recv()

    while True:
        print("set send_ident and send_msg.")
        send_ident = bytes(input("send_ident:"), encoding='utf8')
        send_msg = bytes(input("send_msg:"), encoding='utf8')
        c.Send(send_ident, send_msg)
